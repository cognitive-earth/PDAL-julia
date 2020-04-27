/******************************************************************************
* Copyright (c) 2020, Julian Fell (hi@jtfell.com)
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following
* conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in
*       the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of Hobu, Inc. or Flaxen Geo Consulting nor the
*       names of its contributors may be used to endorse or promote
*       products derived from this software without specific prior
*       written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
****************************************************************************/

#include "Invocation.hpp"

#include <pdal/util/Algorithm.hpp>
#include <pdal/util/FileUtils.hpp>
#include <julia.h>

#ifdef _WIN32
  #include <Windows.h>
#else
  #include <dlfcn.h>
#endif

namespace pdal
{

namespace jlang
{

Invocation::Invocation(const Script& script, MetadataNode m,
        const std::string& pdalArgs) :
    m_script(script), m_inputMetadata(m), m_pdalargs(pdalArgs)
{
    // Environment::get();
    initialise();
    compile();
}

/*
 * Setup the Julia context
 */
void Invocation::initialise()
{
    // dynamically load this same module into itself. PDAL doesn't set the RTLD_GLOBAL flag
    // so Julia doesn't initialise correctly. This can be removed if 
    //
    // https://github.com/PDAL/PDAL/blob/master/pdal/DynamicLibrary.cpp#L96
    //
    // is changed to add that flag. See details here:
    //
    // https://discourse.julialang.org/t/different-behaviours-in-linux-and-macos-with-julia-embedded-in-c/18101/15
    //
    void *handle;
    handle = dlopen("libpdal_plugin_filter_julia.so", RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        fprintf (stderr, "%s\n", dlerror());
        exit(1);
    }

    // TODO Fix this
    //
    //  On debian/ubuntu the path to the Julia runtime is wrong
    //     See issue here:
    //     https://github.com/Non-Contradiction/JuliaCall/issues/99
    //
    //  Can get the correct path during build time (in CMakeLists.txt)
    //    "${Julia_LIBRARY_DIR}/julia"
    //
    // jl_init();
    jl_init_with_image("/usr/lib/x86_64-linux-gnu/julia/", "sys.so");
}

void Invocation::compile()
{
    std::string wrapperModuleSrc = FileUtils::readFileIntoString(FileUtils::toAbsolutePath("../jl/PdalJulia.jl"));
    jl_eval_string(wrapperModuleSrc.c_str());
    m_wrapperMod = (jl_value_t*) jl_eval_string("PdalJulia");

    // Initialise user-supplied script
    jl_eval_string(m_script.source());
    jl_value_t * mod = (jl_value_t*) jl_eval_string(m_script.module());
    m_function = jl_get_function((jl_module_t*) mod, m_script.function());
    // TODO: Check its callable so we fail early
}

jl_array_t* Invocation::prepareData(PointViewPtr& view)
{
    PointLayoutPtr layout(view->table().layout());
    Dimension::IdList const& dims = layout->dims();

    // Allocate the array of arguments as a Julia array
    jl_array_t* arg_array = jl_alloc_vec_any(0);
    // As soon as you have this `jl_array_t`, you need to protect it
    // immediately, before any other `jl_` functions are called.
    // Any call to a `jl_*` function may cause arg_array to be freed
    // behind your back.

    // Luckily, anything stored in arg_array will be known to the GC
    // via the arg_array root so you don't need to root it separately.
    JL_GC_PUSH1(&arg_array);

		m_numDims = 0;
    m_dimNames.clear();
    for (auto di = dims.begin(); di != dims.end(); ++di)
    {
        Dimension::Id d = *di;
        const Dimension::Detail *dd = layout->dimDetail(d);
        const Dimension::Type type = dd->type();

        // Ignore non-floats for now
        // TODO: Fix this!
        if (dd->size() != 8) {
          continue;
        }

        m_numDims++;
        void *data = malloc(dd->size() * view->size());
        char *p = (char *)data;
        for (PointId idx = 0; idx < view->size(); ++idx)
        {
            view->getField(p, d, type, idx);
            p += dd->size();
        }

        if (dd->size() == 8) {
          jl_value_t* array_type = jl_apply_array_type((jl_value_t*) jl_float64_type, 1);
          jl_array_t* array_ptr = jl_ptr_to_array_1d(array_type, data, view->size(), 0);
          // Add the array to the array of arguments
          jl_array_ptr_1d_push(arg_array, (jl_value_t*) array_ptr);
        }
        // TODO: Handle other datatypes

        std::string name = layout->dimName(*di);
        m_dimNames.push_back(name);
    }

    // Allocate array for all the string data, another for pointers to the start of each string
    char** dimNamesArray = (char **) malloc(m_dimNames.size() * sizeof(char*));
    uint32_t full_size = 0;
    for (uint32_t i = 0; i < m_dimNames.size(); i++) {
        full_size += m_dimNames[i].size() * sizeof(char);
    }

    // Copy dim names array into c-style string arrays
    char* dataArray = (char *) malloc(full_size);
    char* headPtr = dataArray;
    for (uint32_t i = 0; i < m_dimNames.size(); i++) {
        dimNamesArray[i] = headPtr;
        strcpy(headPtr, m_dimNames[i].c_str());
        headPtr += m_dimNames[i].size();
    }

    // pointers to start of string
    jl_value_t* array_type_pointer = jl_apply_array_type((jl_value_t*) jl_voidpointer_type, 1);
    jl_array_t* str_array = jl_ptr_to_array_1d( array_type_pointer, (void*) dimNamesArray, m_dimNames.size(), 1);
    jl_array_ptr_1d_push(arg_array, (jl_value_t *) str_array);

    // string contents
    jl_value_t* array_type_uint8 = jl_apply_array_type((jl_value_t*) jl_uint8_type, 1);
    jl_array_t* chr_array = jl_ptr_to_array_1d( array_type_uint8, (uint8_t*) dataArray, full_size, 1 );
    jl_array_ptr_1d_push(arg_array, (jl_value_t *) chr_array);

    // TODO: Inject this into the Julia scope as global objects
    MetadataNode layoutMeta = view->layout()->toMetadata();
    MetadataNode srsMeta = view->spatialReference().toMetadata();

    // addGlobalObject(m_module, plang::fromMetadata(m_inputMetadata), "metadata");
    // addGlobalObject(m_module, getPyJSON(m_pdalargs), "pdalargs");
    // addGlobalObject(m_module, getPyJSON(Utils::toJSON(layoutMeta)), "schema");
    // addGlobalObject(m_module, getPyJSON(Utils::toJSON(srsMeta)),
        // "spatialreference");

    // Critically important: you must pair a POP with every PUSH
    JL_GC_POP();

    // Also critically important: now that you've POP'd,
    // arg_array is no longer rooted, so you should
    // avoid calling any other `jl_` function until it is
    // rooted again by Invocation::execute()
    return arg_array;
}

bool Invocation::execute(PointViewPtr& view, MetadataNode stageMetadata)
{
  // Get the array of arrays representing the PointCloud dimensions ready to be passed into the
  // Julia interpreter
  jl_array_t * julia_args = prepareData(view);

  // Immediately re-protect the args array from the Julia GC
  JL_GC_PUSH1(&julia_args);

  // Add the user-supplied function as the final argument
  jl_array_ptr_1d_push(julia_args, (jl_value_t *) m_function);

  // Run the Julia runtime function "runStage" which:
  //
  // 1. Converts the passed in arrays into a TypedTable form
  // 2. Passes that into the user-supplied function
  // 3. Unpacks the returned `TypedTable` into an array of arrays of dimensions, with the final
  //    array being the strings of the dimensions in order as they preceded it in the array
  jl_function_t* run_stage_fn = jl_get_function((jl_module_t*) m_wrapperMod, "runStage");

  jl_array_t *wrapped_pc = (jl_array_t*) jl_call1(run_stage_fn, (jl_value_t*) julia_args);
  if (jl_exception_occurred())
      std::cout << "Julia Error in runStage: |" << jl_typeof_str(jl_exception_occurred()) << "|\n";

  //
  // Extract the values out of the Julia wrapped types
  //
  assert(jl_is_array(wrapped_pc));
  assert(jl_array_eltype((jl_value_t*) wrapped_pc) == jl_any_type);
  int num_elems = jl_array_dim0(wrapped_pc);
  int num_dims = num_elems - 1;

  // Unpack the list of dim names
  jl_value_t* dim_names_arr = jl_array_ptr_ref(wrapped_pc, num_elems - 1);
  assert(jl_is_array(jl_array_ptr_ref(dim_names_arr, 0)));
  assert(jl_array_dim0(dim_names_arr) == num_dims);

  PointLayoutPtr layout(view->table().layout());

  // Get each dimension (name and array of values)
  for (int dim_index = 0; dim_index < num_dims; dim_index++) {
      jl_value_t* arr = jl_array_ptr_ref(wrapped_pc, dim_index);
      char* dim_name_str = (char *) jl_string_ptr(jl_array_ptr_ref(dim_names_arr, dim_index));

      Dimension::Id d = layout->findDim(dim_name_str);
      const Dimension::Detail *dd = layout->dimDetail(d);
      assert(dd->size() == 8); // floats have a size of 8

      // TODO: Handle other data types here
      if (jl_array_eltype(arr) == jl_float64_type) {
        // Get array pointer
        double *ptr = (double *) jl_array_data(arr);
        // Get number of points
        int point_num = jl_array_dim0(arr);

        // Copy each point into the PDAL view
        for (PointId idx = 0; idx < point_num; ++idx) {
          view->setField(d, idx, ptr[idx]);
        }
      }
  }

  // Critically important: you must pair a POP with every PUSH
  JL_GC_POP();

  return true;

  // TODO: This needs to be called at the very end (not here as this is run for every point cloud view)
  // jl_atexit_hook(0);
}

} // namespace jlang

} // namespace pdal

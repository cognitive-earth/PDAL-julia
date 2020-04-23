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

namespace pdal
{
namespace jlang
{
Invocation::Invocation(const Script& script, MetadataNode m,
        const std::string& pdalArgs) :
    m_script(script), m_inputMetadata(m), m_pdalargs(pdalArgs)
{
    // Environment::get();
    compile();
}


void Invocation::compile()
{
    // Setup the Julia context
    // jl_init();

    // TODO Fix this
    //
    //  On debian/ubuntu the path to the Julia runtime is wrong
    //     See issue here:
    //     https://github.com/Non-Contradiction/JuliaCall/issues/99
    //
    //  Can get the correct path during build time (in CMakeLists.txt)
    //    "${Julia_LIBRARY_DIR}/julia"
    //
    jl_init_with_image("/usr/lib/x86_64-linux-gnu/julia/", "sys.so");

    // Initialise a dictionary of references to make sure julia's GC doesn't free anything we want to keep
    m_juliaGcRefsDict = jl_eval_string("GC_refs = IdDict()");

    std::string wrapperModuleSrc = FileUtils::readFileIntoString(FileUtils::toAbsolutePath("../jl/PdalJulia.jl"));
    jl_eval_string(wrapperModuleSrc.c_str());
    m_wrapperMod = (jl_value_t*) jl_eval_string("PdalJulia");

    // Initialise user-supplied script
    jl_eval_string(m_script.source());
    jl_value_t * mod = (jl_value_t*) jl_eval_string(m_script.module());
    m_function = jl_get_function((jl_module_t*) mod, m_script.function());
    // TODO: Check its callable so we fail early
}

std::vector<jl_value_t **> Invocation::prepareData(PointViewPtr& view)
{
    PointLayoutPtr layout(view->table().layout());
    Dimension::IdList const& dims = layout->dims();

    std::vector<jl_value_t **> arrayBuffers;
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

        // TODO:
        //
        // There are lots of allocations here that need to be dealt with. I believe there are 2 options here:
        //
        // 1. Manually free anything that isn't passed into Julia
        // 2. Use https://github.com/JuliaLang/julia/blob/master/src/julia_gcext.h to allow the Julia GC to discover
        //    roots

        m_numDims++;
        void *data = malloc(dd->size() * view->size());
        char *p = (char *)data;
        for (PointId idx = 0; idx < view->size(); ++idx)
        {
            view->getField(p, d, type, idx);
            p += dd->size();
        }

        // TODO: how do we introspect the type here?
        jl_value_t* array_type = jl_apply_array_type((jl_value_t*) jl_float64_type, 1);
        // jl_value_t* array_type = jl_apply_array_type(reinterpret_cast<jl_value_t *>(jl_voidpointer_type), 1);
        jl_array_t* array_ptr = jl_ptr_to_array_1d(array_type, data, view->size(), 0);
        arrayBuffers.push_back((jl_value_t **) array_ptr);

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
    arrayBuffers.push_back((jl_value_t **) str_array);

    // string contents
    jl_value_t* array_type_uint8 = jl_apply_array_type((jl_value_t*) jl_uint8_type, 1);
    jl_array_t* chr_array = jl_ptr_to_array_1d( array_type_uint8, (uint8_t*) dataArray, full_size, 1 );
    arrayBuffers.push_back((jl_value_t **) chr_array);

    // TODO: Inject this into the Julia scope as global objects
    MetadataNode layoutMeta = view->layout()->toMetadata();
    MetadataNode srsMeta = view->spatialReference().toMetadata();

    // addGlobalObject(m_module, plang::fromMetadata(m_inputMetadata), "metadata");
    // addGlobalObject(m_module, getPyJSON(m_pdalargs), "pdalargs");
    // addGlobalObject(m_module, getPyJSON(Utils::toJSON(layoutMeta)), "schema");
    // addGlobalObject(m_module, getPyJSON(Utils::toJSON(srsMeta)),
        // "spatialreference");

    return arrayBuffers;
}

bool Invocation::execute(PointViewPtr& v, MetadataNode stageMetadata)
{
  // Get the array of arrays representing the PointCloud dimensions ready to be passed into the
  // Julia interpreter
  std::vector<jl_value_t **> juliaArgs = prepareData(v);

  // Add the user-supplied function as the final argument passed to the runStage function in Julia
  juliaArgs.push_back((jl_value_t **) m_function);

  // Run the Julia runtime function "runStage" which:
  //
  // 1. Converts the passed in arrays into a TypedTable form
  // 2. Passes that into the user-supplied function
  // 3. Unpacks the returned `TypedTable` into an array of arrays of dimensions, with the final
  //    array being the strings of the dimensions in order as they preceded it in the array
  jl_function_t* runStageFn = jl_get_function((jl_module_t*) m_wrapperMod, "runStage");
  jl_value_t *wrappedPc = jl_call(runStageFn, (jl_value_t**) juliaArgs.data(), m_numDims + 3);
  if (jl_exception_occurred())
      std::cout << "Julia Error in runStage: |" << jl_typeof_str(jl_exception_occurred()) << "|\n";
  

  return true;

  // TODO: This needs to be called at the very end (not here as this is run for every point cloud view)
  // jl_atexit_hook(0);
}


} // namespace jlang
} // namespace pdal

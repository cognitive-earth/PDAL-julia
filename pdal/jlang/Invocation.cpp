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
    // PyObject *bytecode = Py_CompileString(m_script.source(), m_script.module(),
    //     Py_file_input);
    // if (!bytecode)
    //     throw pdal_error(getTraceback());
    //
    // m_module = PyImport_ExecCodeModule(const_cast<char*>(m_script.module()),
    //     bytecode);
    //
    // Py_DECREF(bytecode);
    // if (!m_module)
    //     throw pdal_error(getTraceback());
    //
    // PyObject *dictionary = PyModule_GetDict(m_module);
    // if (!dictionary)
    //     throw pdal_error("Unable to fetch module dictionary");
    //
    // // Owned by the module through the dictionary.
    // m_function = PyDict_GetItemString(dictionary, m_script.function());
    // if (!m_function)
    // {
    //     std::ostringstream oss;
    //     oss << "unable to find target function '" << m_script.function() <<
    //         "' in module '" << m_script.module() << "'";
    //     throw pdal_error(oss.str());
    // }
    //
    // if (!PyCallable_Check(m_function))
    //     throw pdal_error(getTraceback());


    /* required: setup the Julia context */
  // #ifdef JULIA_SYS_PATH
    /* There is an issue with debian install of julia: https://github.com/Non-Contradiction/JuliaCall/issues/99 */
    // jl_init_with_image(JULIA_SYS_PATH, "sys.so");
  // #else
  // jl_init();
  // #endif
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

std::vector<jl_array_t *> Invocation::prepareData(PointViewPtr& view)
{
    PointLayoutPtr layout(view->table().layout());
    Dimension::IdList const& dims = layout->dims();

    std::vector<jl_array_t *> arrayBuffers;
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

        // TODO: how do we introspect the type here?
        jl_value_t* array_type = jl_apply_array_type((jl_value_t*) jl_float64_type, 1);
        // jl_value_t* array_type = jl_apply_array_type(reinterpret_cast<jl_value_t *>(jl_voidpointer_type), 1);
        jl_array_t* array_ptr = jl_ptr_to_array_1d(array_type, data, view->size(), 0);
        arrayBuffers.push_back(array_ptr);

        std::string name = layout->dimName(*di);
        m_dimNames.push_back(name);
    }

    //
    // The final array in the input is always a list of dimension names for the preceding arrays
    //

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
    arrayBuffers.push_back(str_array);

    // string contents
    jl_value_t* array_type_uint8 = jl_apply_array_type((jl_value_t*) jl_uint8_type, 1);
    jl_array_t* chr_array = jl_ptr_to_array_1d( array_type_uint8, (uint8_t*) dataArray, full_size, 1 );
    arrayBuffers.push_back(chr_array);

    MetadataNode layoutMeta = view->layout()->toMetadata();
    MetadataNode srsMeta = view->spatialReference().toMetadata();

    // TODO: Inject this into the Julia scope as global objects

    // addGlobalObject(m_module, plang::fromMetadata(m_inputMetadata), "metadata");
    // addGlobalObject(m_module, getPyJSON(m_pdalargs), "pdalargs");
    // addGlobalObject(m_module, getPyJSON(Utils::toJSON(layoutMeta)), "schema");
    // addGlobalObject(m_module, getPyJSON(Utils::toJSON(srsMeta)),
        // "spatialreference");

    return arrayBuffers;
}

bool Invocation::execute(PointViewPtr& v, MetadataNode stageMetadata)
{
  std::vector<jl_array_t *> arrayBuffers = prepareData(v);

  jl_function_t* setindex = jl_get_function(jl_base_module, "setindex!");

  // Call the wrapper fn to convert input PC to rich Julia type
  jl_function_t* wrapArgsFn = jl_get_function((jl_module_t*) m_wrapperMod, "wrapArgs");
  jl_value_t *wrappedPc = jl_call(wrapArgsFn, (jl_value_t**) arrayBuffers.data(), m_numDims + 2);
  if (jl_exception_occurred())
      std::cout << "Julia Error in wrapArgs: |" << jl_typeof_str(jl_exception_occurred()) << "|\n";

  // Pass the input into the user-supplied script
  jl_value_t *wrappedPcRet = (jl_value_t*)jl_call1(m_function,(jl_value_t*) wrappedPc);
  // Protect the result from Julia's GC
  jl_call3(setindex, m_juliaGcRefsDict, wrappedPcRet, wrappedPcRet);
  if (jl_exception_occurred())
      std::cout << "Julia Error in user script: |" << jl_typeof_str(jl_exception_occurred()) << "|\n";

  // TODO: Why does this trigger a "BoundsError"?
  // Call the wrapper fn to convert from the rich Julia type back into c++ arrays
  jl_function_t* unwrapRetFn = jl_get_function((jl_module_t*) m_wrapperMod, "unwrapRet");
  jl_value_t *unwrappedArrays = jl_call1(wrapArgsFn, wrappedPcRet);
  if (jl_exception_occurred())
      std::cout << "Julia Error in unwrapRet: |" << jl_typeof_str(jl_exception_occurred()) << "|\n";

  return true;

  // TODO: This needs to be called at the very end (not here as this is run for every point cloud view)
  // jl_atexit_hook(0);

    
    // if (!m_module)
    //     throw pdal_error("No code has been compiled");

    // PyObject *inArrays = prepareData(v);
    // PyObject *outArrays(nullptr);
    //
    // // New object.
    // Py_ssize_t numArgs = argCount(m_function);
    // PyObject *scriptArgs = PyTuple_New(numArgs);
    //
    // if (numArgs > 2)
    //     throw pdal_error("Only two arguments -- ins and outs "
    //         "numpy arrays -- can be passed!");
    //
    // // inArrays and outArrays are owned by scriptArgs here.
    // PyTuple_SetItem(scriptArgs, 0, inArrays);
    // if (numArgs > 1)
    // {
    //     outArrays = PyDict_New();
    //     PyTuple_SetItem(scriptArgs, 1, outArrays);
    // }
    //
    // PyObject *scriptResult = PyObject_CallObject(m_function, scriptArgs);
    // if (!scriptResult)
    //     throw pdal_error(getTraceback());
    // if (!PyBool_Check(scriptResult))
    //     throw pdal_error("User function return value not boolean.");
    //
    // PyObject *maskArray = PyDict_GetItemString(outArrays, "Mask");
    // if (maskArray)
    // {
    //     if (PyDict_Size(outArrays) > 1)
    //         throw pdal_error("'Mask' output array must be the only "
    //             "output array.");
    //     v = maskData(v, maskArray);
    // }
    // else
    //     extractData(v, outArrays);
    // extractMetadata(stageMetadata);
    //
    // // This looks weird, but booleans are implemented as static objects,
    // // allowing this comparison (Py_True is a pointer to the "true" object.)
    // bool ok = scriptResult == Py_True;
    //
    // Py_DECREF(scriptArgs);
    // Py_DECREF(scriptResult);
    // return ok;
}


} // namespace jlang
} // namespace pdal

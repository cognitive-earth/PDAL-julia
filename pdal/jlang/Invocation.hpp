/*****************************************************************************
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

#pragma once

#include <julia.h>
#include <pdal/pdal_internal.hpp>

#include "Script.hpp"

#include <pdal/Dimension.hpp>
#include <pdal/PointView.hpp>

namespace pdal
{
namespace jlang
{

class PDAL_DLL Invocation
{
public:
    Invocation(const Script&, MetadataNode m, const std::string& pdalArgs);
    Invocation& operator=(Invocation const& rhs) = delete;
    Invocation(const Invocation& other) = delete;
    ~Invocation()
    {}

    bool execute(PointViewPtr& v, MetadataNode stageMetadata);

    jl_function_t* m_function;
    jl_value_t* m_wrapperMod;

private:
    void initialise();
    void compile();
    jl_array_t* prepare_data(PointViewPtr& view);
    jl_value_t* determine_jl_type(const Dimension::Detail* dd);
    void unpack_array_into_pdal_view(jl_value_t* arr, PointViewPtr& view, Dimension::Id d);

    Script m_script;

    std::vector<std::string> m_dimNames;
    int32_t m_numDims;

    MetadataNode m_inputMetadata;
    std::string m_pdalargs;
};

} // namespace jlang
} // namespace pdal


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

#include <pdal/pdal_test_main.hpp>

#include <pdal/PipelineManager.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/io/FauxReader.hpp>
#include <pdal/filters/StatsFilter.hpp>
#include <pdal/util/FileUtils.hpp>

#include "../jlang/Invocation.hpp"

#include <pdal/StageWrapper.hpp>

#include "Support.hpp"

using namespace pdal;
using namespace pdal::plang;


class JuliaFilterTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
    }

};

TEST_F(JuliaFilterTest, JuliaFilterTest_test1)
{
    StageFactory f;

    BOX3D bounds1(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    BOX3D bounds2(10.0, 10.0, 10.0, 11.0, 11.0, 11.0);
    FauxReader reader1;
    FauxReader reader2;

    Options ops1;
    ops1.add("bounds", bounds1);
    ops1.add("count", 10);
    ops1.add("mode", "ramp");
    reader1.setOptions(ops1);

    Options ops2;
    ops2.add("bounds", bounds2);
    ops2.add("count", 10);
    ops2.add("mode", "ramp");
    reader2.setOptions(ops2);

    Option source("source", "module MyModule\n"
                   "  function myfunc(xs, ys, zs, t)\n"
                   "    print(xs)\n"
                   "    print(ys)\n"
                   "    print(zs)\n"
                   "    print(t)\n"
                   "  end\n"
                   "end\n");
    Option module("module", "MyModule");
    Option function("function", "myfunc");
    Options opts;
    opts.add(source);
    opts.add(module);
    opts.add(function);

    Stage* filter(f.createStage("filters.julia"));
    if (!filter)
        throw pdal::pdal_error("Unable to create filters.julia");
    filter->setOptions(opts);
    filter->setInput(reader1);
    filter->setInput(reader2);

    std::unique_ptr<StatsFilter> stats(new StatsFilter);
    stats->setInput(*filter);

    PointTable table;

    stats->prepare(table);
    PointViewSet viewSet = stats->execute(table);
    EXPECT_EQ(viewSet.size(), 2u);

    const stats::Summary& statsX = stats->getStats(Dimension::Id::X);
    const stats::Summary& statsY = stats->getStats(Dimension::Id::Y);
    const stats::Summary& statsZ = stats->getStats(Dimension::Id::Z);

    EXPECT_DOUBLE_EQ(statsX.minimum(), 10.0);
    EXPECT_DOUBLE_EQ(statsX.maximum(), 21.0);

    EXPECT_DOUBLE_EQ(statsY.minimum(), 0.0);
    EXPECT_DOUBLE_EQ(statsY.maximum(), 11.0);

    EXPECT_DOUBLE_EQ(statsZ.minimum(), 3.14);
    EXPECT_DOUBLE_EQ(statsZ.maximum(), 3.14);
}


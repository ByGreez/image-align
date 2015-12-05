/**
 This file is part of Image Alignment.
 
 Copyright Christoph Heindl 2015
 
 Image Alignment is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 Image Alignment is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with Image Alignment.  If not, see <http://www.gnu.org/licenses/>.
*/

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <imagealign/warp.h>

TEST_CASE("warp-translational")
{
    namespace ia = imagealign;
    
    typedef ia::Warp<ia::WARP_TRANSLATION> WarpType;
    typedef ia::WarpTraits<ia::WARP_TRANSLATION> Traits;
    
    WarpType w;
    w.setIdentity();
    
    REQUIRE(w.getParameters()(0,0) == 0.f);
    REQUIRE(w.getParameters()(1,0) == 0.f);
    
    Traits::ParamType p;
    p(0,0) = 10.f;
    p(1,0) = 5.f;
    w.setParameters(p);
    
    cv::Point2f x(5.f, 5.f);
    cv::Point2f wx = w(x);
    
    REQUIRE(wx.x == 15.f);
    REQUIRE(wx.y == 10.f);
    
    
    cv::Matx<float, 2, 2> j;
    j << 1, 0, 0, 1;
    REQUIRE(cv::norm(w.jacobian(cv::Point2f(10,10)) - j) == Catch::Detail::Approx(0));
}

TEST_CASE("warp-euclidean")
{
    namespace ia = imagealign;
    
    typedef ia::Warp<ia::WARP_EUCLIDEAN> WarpType;
    typedef ia::WarpTraits<ia::WARP_EUCLIDEAN> Traits;
    
    WarpType w;
    w.setIdentity();
    
    REQUIRE(w.getParameters()(0,0) == 0.f);
    REQUIRE(w.getParameters()(1,0) == 0.f);
    REQUIRE(w.getParameters()(2,0) == 0.f);
    
    Traits::ParamType p;
    p(0,0) = 5.f;
    p(1,0) = 5.f;
    p(2,0) = 3.1415f;
    w.setParameters(p);
    
    cv::Point2f x(0.f, 0.f);
    cv::Point2f wx = w(x);
    
    REQUIRE(wx.x == 5.f);
    REQUIRE(wx.y == 5.f);
    
    
    x = cv::Point2f(10.f, 15.f);
    wx = w(x);
    
    REQUIRE(wx.x == Catch::Detail::Approx(-10.f + 5.f).epsilon(0.01));
    REQUIRE(wx.y == Catch::Detail::Approx(-15.f + 5.f).epsilon(0.01));
}
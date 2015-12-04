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

#ifndef IMAGE_ALIGN_FORWARD_ADDITIVE_H
#define IMAGE_ALIGN_FORWARD_ADDITIVE_H

#include <imagealign/warp.h>
#include <imagealign/bilinear.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

namespace imagealign {
    
    /** 
        Forward-additive image alignment.
        
        'Best' aligns a template image with a target image through minimization of the sum of 
        squared intensity errors between the warped target image and the template image with 
        respect to the warp parameters.
     
        This algorithm is the classic algorithm proposed by Lucas-Kanade. Baker and Matthews 
        coined it later the forwards-additive algorithm because of its properties that the 
        direction of the warp is forward and warp parameters are summed.
     
        \tparam WarpType Type of warp motion to use during alignment. See EWarpType.
     
        ## Notes
     
        The implementation is not trimmed towards runtime nor memory efficiency. Some images
        are explicitely created for the sake of readability of the code.
     
        ## Based on
     
        [1] Lucas, Bruce D., and Takeo Kanade.
        "An iterative image registration technique with an application to stereo vision." 
        IJCAI. Vol. 81. 1981.
     
        [2] Baker, Simon, and Iain Matthews.
        "Lucas-kanade 20 years on: A unifying framework." 
        International journal of computer vision 56.3 (2004): 221-255.

     */
    template<int WarpType>
    class AlignForwardAdditive {
    public:
        enum {
            NParameters = Warp<WarpType>::NParameters
        };
        
        /** The type of warp. */
        typedef Warp<WarpType> WType;
        
        /** Type to hold Hessian matrix */
        typedef cv::Matx<float, NParameters, NParameters> HessianType;
        
        /** Type to hold the steepest descent image for a single pixel */
        typedef cv::Matx<float, 1, NParameters> PixelSDIType;
        
        /** Type to hold the transposed steepest descent image for a single pixel */
        typedef cv::Matx<float, NParameters, 1> PixelSDITransposedType;
        
        
        /** 
            Prepare for alignment.
         
            This function takes the template and target image and performs
            necessary pre-calculations to speed up the alignment process.
         
            \param tmpl Single channel template image
            \param target Single channel target image to align template with.
         */
        void prepare(cv::InputArray tmpl, cv::InputArray target)
        {
            CV_Assert(tmpl.channels() == 1);
            CV_Assert(target.channels() == 1);
            
            tmpl.getMat().convertTo(_template, CV_32F);
            target.getMat().convertTo(_target, CV_32F);
            
            _warpedTarget.create(_template.size(), CV_32FC1);
            _errorImage.create(_template.size(), CV_32FC1);
            _warpedGradX.create(_template.size(), CV_32FC1);
            _warpedGradY.create(_template.size(), CV_32FC1);
            
            cv::Sobel(_target, _gradX, CV_32F, 1, 0);
            cv::Sobel(_target, _gradY, CV_32F, 0, 1);
            
            // Sobel uses 3x3 convolution matrix and result is not in units
            // of intensity anymore. Hence, normalize
            _gradX *= 0.125f;
            _gradY *= 0.125f;
        }
        
        /** Perform a single alignment step. 
         
            This method takes the current state of the warp parameters and refines
            them by minimizing the sum of squared intensity differences.
         
            \param w Current state of warp estimation. Will be modified to hold result.
         */
        float align(WType &w)
        {
            // Warp target back to template with respect to current warp parameters
            warpImage<float>(_target, _warpedTarget, _warpedTarget.size(), w);
            
            // Warp the gradient
            warpImage<float>(_gradX, _warpedGradX, _warpedGradX.size(), w);
            warpImage<float>(_gradY, _warpedGradY, _warpedGradY.size(), w);
            
            // Compute the error image
            _errorImage = _template - _warpedTarget;
            
            // Compute the Jacobian of the warp
            typename WType::JType jacobian = w.jacobian();
            
            HessianType hessian = HessianType::zeros();
            PixelSDITransposedType sumSDITimesError = PixelSDITransposedType::zeros();
            
            // Loop over template region
            for (int y = 0; y < _template.rows; ++y) {
                
                const float *gxRow = _warpedGradX.ptr<float>(y);
                const float *gyRow = _warpedGradY.ptr<float>(y);
                const float *eRow = _errorImage.ptr<float>(y);
                
                for (int x = 0; x < _template.cols; ++x) {
                    const PixelSDIType sd = cv::Matx<float, 1, 2>(gxRow[x], gyRow[x]) * jacobian;
                    sumSDITimesError += (sd.t() * eRow[x]);
                    hessian += sd.t() * sd;
                }
            }
            
            typename WType::VType delta = hessian.inv() * sumSDITimesError;
            
            // Additive warp parameter update.
            w.setParameters(w.getParameters() + delta);
            
            // Return the average intensity error.
            return cv::mean(_errorImage)[0];
        }
        
    private:
        
        cv::Mat _template;
        cv::Mat _target;
        cv::Mat _warpedTarget;
        cv::Mat _errorImage;
        cv::Mat _gradX, _gradY;
        cv::Mat _warpedGradX, _warpedGradY;
    };
    
    
}

#endif
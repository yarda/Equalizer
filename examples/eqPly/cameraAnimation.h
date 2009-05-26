/* Copyright (c) 2009, Makhinya Maxim
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 *  
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef EQ_PLY_CAMERAANIMATION_H
#define EQ_PLY_CAMERAANIMATION_H

#include <vmmlib/vmmlib.h>
#include <vmmlib/vector3.h>
#include <eq/base/base.h>
#include <math.h>

namespace eqPly
{

    /**
     * Loads sequence of camera positions and interpolates them on a per-frame
     * basis.
     */
    class CameraAnimation
    {
    public:
        struct Step;

        CameraAnimation() : _curStep( 0 ), _curFrame( 0 ) {}

        bool loadAnimation( const std::string& fileName );

        bool isValid() const { return !_steps.empty(); }

        Step getNextStep();

        const vmml::Vector3f& getModelRotation() const { return _modelRotation;}

        struct Step
        {
            Step()
                : frame( 0 )
                , translation( vmml::Vector3f( .0f, .0f, -1.0f ))
                , rotation(    vmml::Vector3f( .0f, .0f,   .0f )){}

            Step( int frame_, const vmml::Vector3f& translation_,
                              const vmml::Vector3f& rotation_  )
                : frame( frame_ )
                , translation( translation_ ),
                  rotation( rotation_ ){}

            int frame;
            vmml::Vector3f translation;
            vmml::Vector3f rotation;
        };

    private:
        vmml::Vector3f      _modelRotation;
        std::vector< Step > _steps;
        uint32_t            _curStep;
        int32_t             _curFrame;
    };

}

#endif // EQ_PLY_CAMERAANIMATION_H

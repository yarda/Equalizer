
/* Copyright (c) 2009, Stefan Eilemann <eile@equalizergraphics.com>
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

#include "texture.h"

#include <eq/client/image.h>

namespace eq
{
namespace util
{
Texture::Texture( GLEWContext* const glewContext )
        : _id( 0 )
        , _target( GL_TEXTURE_RECTANGLE_ARB )
        , _internalFormat( 0 )
        , _format( 0 )
        , _type( 0 )
        , _width( 0 )
        , _height( 0 )
        , _defined( false ) 
        , _glewContext( glewContext )
{
}

Texture::~Texture()
{
    if( _id != 0 )
        EQWARN << "OpenGL texture was not freed" << std::endl;

    _id      = 0;
    _defined = false;
}

bool Texture::isValid() const
{ 
    return ( _id != 0 && _defined );
}

void Texture::flush()
{
    if( _id == 0 )
        return;

    CHECK_THREAD( _thread );
    glDeleteTextures( 1, &_id );
    _id = 0;
    _defined = false;
}

void Texture::setTarget( const GLenum target )
{               
    _target = target;
}

void Texture::setFormat( const GLuint format )
{
    if( _internalFormat == format )
        return;

    _defined = false;
    _internalFormat = format;

    switch( format )
    {
        // depth format
        case GL_DEPTH_COMPONENT:
            _format = GL_DEPTH_COMPONENT;
            _type   = GL_UNSIGNED_INT;
            break;

        // color formats
        case GL_RGBA8:
        case GL_RGBA16:
        case GL_BGRA:
            _format = GL_BGRA;
            _type   = GL_UNSIGNED_BYTE;
            break;

        case GL_RGBA16F:
            _format = GL_RGBA;
            _type   = GL_HALF_FLOAT;
            break;

        case GL_RGBA32F:
            _format = GL_RGBA;
            _type   = GL_FLOAT;
            break;

        case GL_ALPHA32F_ARB:
            _format = GL_ALPHA;
            _type   = GL_FLOAT;
            break;

        case GL_RGBA32UI:
            if( GLEW_EXT_texture_integer )
            {
                _format = GL_RGBA_INTEGER_EXT;
                _type   = GL_UNSIGNED_INT;
            }
            else
                EQUNIMPLEMENTED;
            break;

        default:
            _format = _internalFormat;
            _type   = GL_UNSIGNED_BYTE;
    }
}

void Texture::_generate()
{
    CHECK_THREAD( _thread );
    if( _id != 0 )
        return;

    _defined = false;
    glGenTextures( 1, &_id );
}

bool Texture::_isDimPOT( const uint32_t width, const uint32_t height )
{
    return ( width > 0 && height > 0 &&
             ( width & ( width - 1 )) == 0 &&
             ( height & ( height - 1 )) == 0 );
}

void Texture::_resize( const int32_t width, const int32_t height )
{
    if( _width < width )
    {
        _width   = width;
        _defined = false;
    }

    if( _height < height )
    {
        _height  = height;
        _defined = false;
    }
}

void Texture::copyFromFrameBuffer( const PixelViewport& pvp )
{
    CHECK_THREAD( _thread );
    EQASSERT( _internalFormat != 0 );

    _generate();
    _resize( pvp.w, pvp.h );
    glBindTexture( _target, _id );

    if( !_defined )
        resize( _width, _height );

    EQ_GL_CALL( glCopyTexSubImage2D( _target, 0, 0, 0,
                                     pvp.x, pvp.y, pvp.w, pvp.h ));
    glFinish();
}

void Texture::upload( const Image* image, const Frame::Buffer which )
{
    CHECK_THREAD( _thread );

    setFormat( image->getInternalTextureFormat( which ));
    EQASSERT( _internalFormat != 0 );

    const eq::PixelViewport& pvp = image->getPixelViewport();

    _format = image->getFormat( which );
    _type = image->getType( which );

    upload( pvp.w, pvp.h, ( void* )image->getPixelPointer( which ));
}

void Texture::upload( const int width, const int height, void* ptr )
{
    _generate();
    _resize( width, height );
    glBindTexture( _target, _id );

    if( !_defined )
        resize( _width, _height );

    EQ_GL_CALL( glTexSubImage2D( _target, 0, 0, 0,
                                 width, height, _format, _type, ptr ));
}

void Texture::download( void* buffer, const uint32_t format,
                        const uint32_t type ) const
{
    CHECK_THREAD( _thread );
    EQASSERT( _defined );
    EQ_GL_CALL( glBindTexture( _target, _id ));
    EQ_GL_CALL( glGetTexImage( _target, 0,
                               format, type, buffer ));
}

void Texture::download( void* buffer ) const
{
    download( buffer, _format, _type );
}

void Texture::bind() const
{
    EQASSERT( _id );
    glBindTexture( _target, _id );
}

void Texture::bindToFBO( const GLenum target, const int width, 
                         const int height )
{
    CHECK_THREAD( _thread );
    EQASSERT( _internalFormat );
    EQASSERT( _glewContext );

    _generate();

    glBindTexture( _target, _id );
    glTexImage2D( _target, 0, _internalFormat, width, height,
                  0, _format, _type, 0 );
    glFramebufferTexture2DEXT( GL_FRAMEBUFFER, target, _target,
                               _id, 0 );

    _width = width;
    _height = height;
    _defined = true;
}

void Texture::resize( const int width, const int height )
{
    CHECK_THREAD( _thread );
    EQASSERT( _id );
    EQASSERT( _internalFormat );
    EQASSERT( width > 0 && height > 0 );

    if( _width == width && _height == height && _defined )
        return;

    if( !_isDimPOT( width, height ))
        EQASSERT( GLEW_ARB_texture_non_power_of_two );

    glBindTexture( _target, _id );
    glTexImage2D( _target, 0, _internalFormat, width, height,
                  0, _format, _type, 0 );

    _width  = width;
    _height = height;
    _defined = true;
}

void Texture::writeTexture( const std::string& filename, 
                            const Frame::Buffer buffer,
                            const PixelViewport& pvp ) const
{
    eq::Image* image = new eq::Image();

    GLuint type;
    switch( getFormat( ))
    {
        case GL_RGBA32F:
            type = GL_FLOAT;
        case GL_RGBA16F:
            type = GL_HALF_FLOAT;
        default:
            type = GL_UNSIGNED_BYTE;
    }

    image->setType( buffer, type );
    image->setFormat( buffer, getFormat() );

    image->setPixelViewport( pvp );
    image->validatePixelData( buffer );

    download( image->getPixelPointer( buffer ), getFormat(), type );

    image->writeImage( filename + ".rgb", buffer );

    delete image;
}

}
}
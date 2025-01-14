
/* Copyright (c) 2007-2011, Stefan Eilemann <eile@equalizergraphics.com>
 *               2009-2010, Cedric Stalder <cedric.stalder@gmail.com> 
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

#ifndef CO_DATAISTREAM_H
#define CO_DATAISTREAM_H

#include <co/api.h>
#include <co/types.h>

#include <co/base/buffer.h> // member
#include <co/base/types.h>


#include <iostream>
#include <vector>

namespace co
{
    /** A std::istream-like input data stream for binary data. */
    class DataIStream
    {
    public:
        /** @name Internal */
        //@{ 
        CO_API DataIStream();
        DataIStream( const DataIStream& );
        CO_API virtual ~DataIStream();

        /** Get the number of remaining buffers. */
        virtual size_t nRemainingBuffers() const = 0;

        virtual uint128_t getVersion() const = 0;

        virtual void reset() { _reset(); }
        //@}

        /** @name Data input */
        //@{
        /** Read a plain data item. */
        template< typename T > DataIStream& operator >> ( T& value )
            { read( &value, sizeof( value )); return *this; }

        /** Read a std::vector of serializable items. */
        template< typename T >
        DataIStream& operator >> ( std::vector< T >& value )
        {
            uint64_t nElems = 0;
            read( &nElems, sizeof( nElems ));
            value.resize( nElems );
            for( uint64_t i = 0; i < nElems; i++ )
                (*this) >> value[i];

            return *this; 
        }

        /**
         * Deserialize child objects.
         *
         * Existing children are synced to the new version. New children are
         * created by calling the <code>void create( C** child )</code> method
         * on the object, and registered or mapped to the object's
         * session. Removed children are released by calling the <code>void
         * release( C* )</code> method on the object. The resulting child vector
         * is created in result. The old and result vector can be the same
         * object, the result vector is cleared and rebuild completely.
         */
        template< typename O, typename C >
        void deserializeChildren( O* object, const std::vector< C* >& old,
                                  std::vector< C* >& result );

        /** Read a number of bytes from the stream into a buffer.  */
        CO_API void read( void* data, uint64_t size );

        /** 
         * Get the pointer to the remaining data in the current buffer.
         *
         * The data written by the DataOStream on the other end is bucketized,
         * that is, it is sent in multiple blocks. The remaining buffer and its
         * size points into one of the buffers, that is, not all the data sent
         * is returned by this function. However, a write operation on the other
         * end is never segmented, that is, if the application writes n bytes to
         * the DataOStream, a symmetric read from the DataIStream has at least n
         * bytes available.
         */
        CO_API const void* getRemainingBuffer();

        /** Get the size of the remaining data in the current buffer. */
        CO_API uint64_t getRemainingBufferSize();

        /** Advance the current buffer by a number of bytes. */
        CO_API void advanceBuffer( const uint64_t offset );

        /** @return true if not all data has been read. */
        bool hasData() { return _checkBuffer(); }
        //@}
 
    protected:
        virtual bool getNextBuffer( uint32_t* compressor, uint32_t* nChunks,
                                    const void** chunkData, uint64_t* size )=0;

    private:
        /** The current input buffer */
        const uint8_t* _input;
        /** The size of the input buffer */
        uint64_t _inputSize;
        /** The current read position in the buffer */
        uint64_t  _position;

        base::CPUCompressor* const _decompressor; //!< current decompressor
        base::Bufferb _data; //!< decompressed buffer

        /**
         * Check that the current buffer has data left, get the next buffer is
         * necessary, return false if no data is left. 
         */
        CO_API bool _checkBuffer();
        CO_API void _reset();
        
        const uint8_t* _decompress( const void* data, const uint32_t name,
                                    const uint32_t nChunks,
                                    const uint64_t dataSize );

        /** Read a vector of trivial data. */
        template< typename T >
        DataIStream& _readFlatVector ( std::vector< T >& value )
        {
            uint64_t nElems = 0;
            read( &nElems, sizeof( nElems ));
            EQASSERTINFO( nElems < EQ_BIT48,
                  "Out-of-sync co::DataIStream: " << nElems << " elements?" );
            value.resize( size_t( nElems ));
            if( nElems > 0 )
                read( &value.front(), nElems * sizeof( T ));
            return *this; 
        }
    };
}

#include <co/object.h>
#include <co/objectVersion.h>

namespace co{
    /** @name Specialized input operators */
    //@{
    /** Read a std::string. */
    template<>
    inline DataIStream& DataIStream::operator >> ( std::string& str )
    { 
        uint64_t nElems = 0;
        read( &nElems, sizeof( nElems ));
        EQASSERTINFO( nElems <= getRemainingBufferSize(),
                      nElems << " > " << getRemainingBufferSize( ));
        if( nElems == 0 )
            str.clear();
        else
        {
            str.assign( static_cast< const char* >( getRemainingBuffer( )), 
                        size_t( nElems ));
            advanceBuffer( nElems );
        }
        return *this; 
    }

    /** Deserialize an object (id+version). */
    template<> inline DataIStream& DataIStream::operator >> ( Object*& object )
    {
        ObjectVersion data;
        (*this) >> data;
        EQASSERT( object->getID() == data.identifier );
        object->sync( data.version );
        return *this;
    }

    namespace
    {
    class ObjectFinder
    {
    public:
        ObjectFinder( const base::UUID& id ) : _id( id ) {}
        bool operator()( co::Object* candidate )
            { return candidate->getID() == _id; }

    private:
        const base::UUID _id;
    };
    }

/** @cond IGNORE */
    template< typename O, typename C > inline void
    DataIStream::deserializeChildren( O* object, const std::vector< C* >& old_,
                                      std::vector< C* >& result )
    {
        ObjectVersions versions;
        (*this) >> versions;
        std::vector< C* > old = old_;

        // rebuild vector from serialized list
        result.clear();
        for( ObjectVersions::const_iterator i = versions.begin();
             i != versions.end(); ++i )
        {
            const ObjectVersion& version = *i;
            
            if( version.identifier == base::UUID::ZERO )
            {
                result.push_back( 0 );
                continue;
            }

            typename std::vector< C* >::iterator j =
                stde::find_if( old, ObjectFinder( version.identifier ));

            if( j == old.end( )) // previously unknown child
            {
                C* child = 0;
                object->create( &child );
                LocalNodePtr localNode = object->getLocalNode();
                EQASSERT( child );
                EQASSERT( !object->isMaster( ));

                EQCHECK( localNode->mapObject( child, version ));
                result.push_back( child );
            }
            else
            {
                C* child = *j;
                old.erase( j );
                if( object->isMaster( ))
                    child->sync( VERSION_HEAD );
                else
                    child->sync( version.version );

                result.push_back( child );
            }
        }

        while( !old.empty( )) // removed children
        {
            C* child = old.back();
            old.pop_back();
            if( !child )
                continue;

            if( child->isAttached() && !child->isMaster( ))
            {
                LocalNodePtr localNode = object->getLocalNode();
                localNode->unmapObject( child );
            }
            object->release( child );
        }
    }
/** @endcond */

    /** Optimized specialization to read a std::vector of uint8_t. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< uint8_t >& value )
    { return _readFlatVector( value );}

    /** Optimized specialization to read a std::vector of uint16_t. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< uint16_t >& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of int16_t. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< int16_t >& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of uint32_t. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< uint32_t >& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of int32_t. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< int32_t >& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of uint64_t. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< uint64_t>& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of int64_t. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< int64_t >& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of float. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< float >& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of double. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< double >& value )
    { return _readFlatVector( value ); }

    /** Optimized specialization to read a std::vector of ObjectVersion. */
    template<> inline DataIStream&
    DataIStream::operator >> ( std::vector< ObjectVersion >& value )
    { return _readFlatVector( value ); }
    //@}
}

#endif //CO_DATAISTREAM_H

/*
 * Ubitrack - Library for Ubiquitous Tracking
 * Copyright 2006, Technische Universitaet Muenchen, and individual
 * contributors as indicated by the @authors tag. See the
 * copyright.txt in the distribution for a full listing of individual
 * contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA, or see the FSF site: http://www.fsf.org.
 */


#ifndef UBITACK_CUSTOMGEOMETRY_H
#define UBITACK_CUSTOMGEOMETRY_H

#include "TrackedObject.h"

#ifdef __APPLE__
    #include <OpenGL/OpenGL.h>
	#include <OpenGL/glu.h>
#else
    #include <GL/gl.h>			// Header File For The OpenGL32 Library
    #include <GL/glu.h>			// Header File For The GLu32 Library
#endif


/* assimp include files. These three are usually needed. */
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Ubitrack { namespace Drivers {

/**
 * @ingroup driver_components
 * Component for rendering custom objects.
 * Provides a push-in port for poses.
 */
class CustomGeometry : public TrackedObject {
public:
    /**
     * Constructor
     * @param name edge name
     * @param config component configuration
     * @param componentKey the unique identifier for this component
     * @param pModule parent object
     */
    CustomGeometry( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph,
            const VirtualObjectKey& componentKey, VirtualCamera* pModule );

    /** render the object, if up-to-date tracking information is available */
    virtual void draw3DContent( Measurement::Timestamp& t, int parity );

    virtual void stop()
    {
        // invoke stop() in superclass
        TrackedObject::stop();

        aiReleaseImport(m_scene);
    }

protected:

    void apply_material(const struct aiMaterial *mtl);
    void recursive_render (const struct aiScene *sc, const struct aiNode* nd);


    // render only into z-buffer for occlusion objects?
    bool m_occlusionOnly;

    std::string m_modelFilePath;

    // geometry parsing/rendering

    const aiScene* m_scene;
    GLuint m_scene_list;

};

} } // namespace Ubitrack::Drivers
#endif //UBITACK_CUSTOMGEOMETRY_H

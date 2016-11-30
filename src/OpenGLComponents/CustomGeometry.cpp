//
// Created by Ulrich Eck on 25/11/2016.
//

#include "CustomGeometry.h"

namespace Ubitrack { namespace Drivers {


/* ---------------------------------------------------------------------------- */
void color4_to_float4(const aiColor4D *c, float f[4])
{
    f[0] = c->r;
    f[1] = c->g;
    f[2] = c->b;
    f[3] = c->a;
}

/* ---------------------------------------------------------------------------- */
void set_float4(float f[4], float a, float b, float c, float d)
{
    f[0] = a;
    f[1] = b;
    f[2] = c;
    f[3] = d;
}


CustomGeometry::CustomGeometry( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph,
        const VirtualObjectKey& componentKey, VirtualCamera* pModule )
        : TrackedObject( name, subgraph, componentKey, pModule )
        , m_modelFilePath( "" )
        , m_occlusionOnly( false )
        , m_scene(NULL)
        , m_scene_list(0)
{
    // load object path

    m_modelFilePath = subgraph->m_DataflowAttributes.getAttribute( "virtualObjectPath" ).getText();
    if ( m_modelFilePath.length() == 0)
        UBITRACK_THROW( "VirtualObject component with empty virtualObjectPath  attribute" );

    if ( subgraph->m_DataflowAttributes.hasAttribute( "occlusionOnly" ) && subgraph->m_DataflowAttributes.getAttribute( "occlusionOnly" ).getText() == "true" )
        m_occlusionOnly = true;

    // load model
    struct aiLogStream stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
    aiAttachLogStream(&stream);

    m_scene = aiImportFile(m_modelFilePath.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);

    aiDetachAllLogStreams();

}

/** render the object, if up-to-date tracking information is available */
void CustomGeometry::draw3DContent( Measurement::Timestamp& t, int parity )
{
    // Remember old blend mode
    GLboolean blendMode[4];

    LOG4CPP_DEBUG( logger, "CustomGeometry::draw3DContent() for timestamp " << t );
    // render only into z-buffer?
    if ( m_occlusionOnly )
    {
        glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
        glGetBooleanv(GL_COLOR_WRITEMASK, blendMode);
    }

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);    /* Uses default lighting parameters */

//    glEnable(GL_DEPTH_TEST);

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glEnable(GL_NORMALIZE);

    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);


    /* if the display list has not been made yet, create a new one and
       fill it with scene contents */
    if(m_scene_list == 0) {
        m_scene_list = glGenLists(1);
        glNewList(m_scene_list, GL_COMPILE);
        /* now begin at the root node of the imported data and traverse
           the scenegraph by multiplying subsequent local transforms
           together on GL's matrix stack. */
        recursive_render(m_scene, m_scene->mRootNode);
        glEndList();
    }

    glCallList(m_scene_list);

    glPopAttrib();

    // Reset old blend mode
    if ( m_occlusionOnly )
        glColorMask( blendMode[0], blendMode[1], blendMode[2], blendMode[3] );
}


void CustomGeometry::apply_material(const aiMaterial *mtl)
{
    float c[4];

    GLenum fill_mode;
    int ret1, ret2;
    aiColor4D diffuse;
    aiColor4D specular;
    aiColor4D ambient;
    aiColor4D emission;
    float shininess, strength;
    int two_sided;
    int wireframe;
    unsigned int max;

    set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse)) {
        color4_to_float4(&diffuse, c);
    }
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);

    set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular)) {
        color4_to_float4(&specular, c);    
    }
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);

    set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient)) {
        color4_to_float4(&ambient, c);
    }
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);

    set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission)) {
        color4_to_float4(&emission, c);
    }
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, c);

    max = 1;
    ret1 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
    if(ret1 == AI_SUCCESS) {
        max = 1;
        ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
        if(ret2 == AI_SUCCESS)
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * strength);
        else
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
    }
    else {
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
        set_float4(c, 0.0f, 0.0f, 0.0f, 0.0f);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
    }

    max = 1;
    if(AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_ENABLE_WIREFRAME, &wireframe, &max))
        fill_mode = wireframe ? GL_LINE : GL_FILL;
    else
        fill_mode = GL_FILL;
    glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

    max = 1;
    if((AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, &max)) && two_sided)
        glDisable(GL_CULL_FACE);
    else
        glEnable(GL_CULL_FACE);
}

void CustomGeometry::recursive_render (const aiScene *sc, const aiNode* nd)
{
    unsigned int i;
    unsigned int n = 0, t;
    aiMatrix4x4 m = nd->mTransformation;

    /* update transform */
    aiTransposeMatrix4(&m);
    glPushMatrix();
    glMultMatrixf((float*)&m);

    /* draw all meshes assigned to this node */
    for (; n < nd->mNumMeshes; ++n) {
        const aiMesh* mesh = m_scene->mMeshes[nd->mMeshes[n]];

        apply_material(sc->mMaterials[mesh->mMaterialIndex]);

        if(mesh->mNormals == NULL) {
            glDisable(GL_LIGHTING);
        } else {
            glEnable(GL_LIGHTING);
        }

        for (t = 0; t < mesh->mNumFaces; ++t) {
            const aiFace* face = &mesh->mFaces[t];
            GLenum face_mode;

            switch(face->mNumIndices) {
            case 1: face_mode = GL_POINTS; break;
            case 2: face_mode = GL_LINES; break;
            case 3: face_mode = GL_TRIANGLES; break;
            default: face_mode = GL_POLYGON; break;
            }

            glBegin(face_mode);

            for(i = 0; i < face->mNumIndices; i++) {
                int index = face->mIndices[i];
                if(mesh->mColors[0] != NULL) {
                    glColor4fv((GLfloat*)&mesh->mColors[0][index]);
                }
                if(mesh->mNormals != NULL) {
                    glNormal3fv(&mesh->mNormals[index].x);
                }
                glVertex3fv(&mesh->mVertices[index].x);
            }

            glEnd();
        }

    }

    /* draw all children */
    for (n = 0; n < nd->mNumChildren; ++n) {
        recursive_render(sc, nd->mChildren[n]);
    }

    glPopMatrix();
}


} } // namespace Ubitrack::Drivers
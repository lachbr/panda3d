//#pragma warning(disable: 4018) // '<' : signed/unsigned mismatch

#include "csg.h"

#include "bspMaterial.h"
#include "keyValues.h"
#include "transformState.h"

int             g_nummapbrushes;
brush_t         g_mapbrushes[MAX_MAP_BRUSHES];

int             g_numbrushsides;
side_t          g_brushsides[MAX_MAP_SIDES];

int             g_nMapFileVersion = 220;

static const vec3_t   s_baseaxis[18] = {
        { 0, 0, 1 },{ 1, 0, 0 },{ 0, -1, 0 },                      // floor
{ 0, 0, -1 },{ 1, 0, 0 },{ 0, -1, 0 },                     // ceiling
{ 1, 0, 0 },{ 0, 1, 0 },{ 0, 0, -1 },                      // west wall
{ -1, 0, 0 },{ 0, 1, 0 },{ 0, 0, -1 },                     // east wall
{ 0, 1, 0 },{ 1, 0, 0 },{ 0, 0, -1 },                      // south wall
{ 0, -1, 0 },{ 1, 0, 0 },{ 0, 0, -1 },                     // north wall
};

int				g_numparsedentities;
int				g_numparsedbrushes;

static pmap<std::string, int> propname_to_propnum;

CPT(TransformState) GetMyTransform(CKeyValues *kv, CPT(TransformState) transform)
{
        CPT(TransformState) my_transform = TransformState::make_identity();
        if (kv->has_key("origin"))
        {
                LPoint3 origin = CKeyValues::to_3f(kv->get_value("origin"));
                my_transform = my_transform->set_pos(origin);
        }
        if (kv->has_key("angles"))
        {
                LVector3 angles = CKeyValues::to_3f(kv->get_value("angles"));
                my_transform = my_transform->set_hpr(angles);
        }
        if (kv->has_key("scale"))
        {
                LVector3 scale = CKeyValues::to_3f(kv->get_value("scale"));
                my_transform = my_transform->set_scale(scale);
        }
        if (kv->has_key("shear"))
        {
                LVector3 shear = CKeyValues::to_3f(kv->get_value("shear"));
                my_transform = my_transform->set_shear(shear);
        }

        my_transform = transform->compose(my_transform);
        return my_transform;
}

brush_t *CopyCurrentBrush( entity_t *entity, const brush_t *brush )
{
        if ( entity->firstbrush + entity->numbrushes != g_nummapbrushes )
        {
                Error( "CopyCurrentBrush: internal error." );
        }
        brush_t *newb = &g_mapbrushes[g_nummapbrushes];
        g_nummapbrushes++;
        hlassume( g_nummapbrushes <= MAX_MAP_BRUSHES, assume_MAX_MAP_BRUSHES );
        //memcpy( newb, brush, sizeof( brush_t ) );
        *newb = *brush;
        newb->firstside = g_numbrushsides;
        g_numbrushsides += brush->numsides;
        hlassume( g_numbrushsides <= MAX_MAP_SIDES, assume_MAX_MAP_SIDES );
        memcpy( &g_brushsides[newb->firstside], &g_brushsides[brush->firstside], brush->numsides * sizeof( side_t ) );
        newb->entitynum = entity - g_bspdata->entities;
        newb->brushnum = entity->numbrushes;
        entity->numbrushes++;
        for ( int h = 0; h < NUM_HULLS; h++ )
        {
                if ( brush->hullshapes[h] != NULL )
                {
                        newb->hullshapes[h] = _strdup( brush->hullshapes[h] );
                }
                else
                {
                        newb->hullshapes[h] = NULL;
                }
        }
        return newb;
}
void DeleteCurrentEntity( entity_t *entity )
{
        if ( entity != &g_bspdata->entities[g_bspdata->numentities - 1] )
        {
                Error( "DeleteCurrentEntity: internal error." );
        }
        if ( entity->firstbrush + entity->numbrushes != g_nummapbrushes )
        {
                Error( "DeleteCurrentEntity: internal error." );
        }
        for ( int i = entity->numbrushes - 1; i >= 0; i-- )
        {
                brush_t *b = &g_mapbrushes[entity->firstbrush + i];
                if ( b->firstside + b->numsides != g_numbrushsides )
                {
                        Error( "DeleteCurrentEntity: internal error. (Entity %i, Brush %i)",
                               b->originalentitynum, b->originalbrushnum
                        );
                }
                memset( &g_brushsides[b->firstside], 0, b->numsides * sizeof( side_t ) );
                g_numbrushsides -= b->numsides;
                for ( int h = 0; h < NUM_HULLS; h++ )
                {
                        if ( b->hullshapes[h] )
                        {
                                free( b->hullshapes[h] );
                        }
                }
        }
        memset( &g_mapbrushes[entity->firstbrush], 0, entity->numbrushes * sizeof( brush_t ) );
        g_nummapbrushes -= entity->numbrushes;
        while ( entity->epairs )
        {
                DeleteKey( entity, entity->epairs->key );
        }
        memset( entity, 0, sizeof( entity_t ) );
        g_bspdata->numentities--;
}
// =====================================================================================
//  TextureAxisFromPlane
// =====================================================================================
void            TextureAxisFromPlane( const plane_t* const pln, vec3_t xv, vec3_t yv )
{
        int             bestaxis;
        vec_t           dot, best;
        int             i;

        best = 0;
        bestaxis = 0;

        for ( i = 0; i < 6; i++ )
        {
                dot = DotProduct( pln->normal, s_baseaxis[i * 3] );
                if ( dot > best )
                {
                        best = dot;
                        bestaxis = i;
                }
        }

        VectorCopy( s_baseaxis[bestaxis * 3 + 1], xv );
        VectorCopy( s_baseaxis[bestaxis * 3 + 2], yv );
}

#define ScaleCorrection	(1.0/128.0)


// =====================================================================================
//  CheckForInvisible
//      see if a brush is part of an invisible entity (KGP)
// =====================================================================================
static bool CheckForInvisible( entity_t* mapent )
{
        using namespace std;

        string keyval( ValueForKey( mapent, "classname" ) );
        if ( g_invisible_items.count( keyval ) )
        {
                return true;
        }

        keyval.assign( ValueForKey( mapent, "targetname" ) );
        if ( g_invisible_items.count( keyval ) )
        {
                return true;
        }

        keyval.assign( ValueForKey( mapent, "zhlt_invisible" ) );
        if ( !keyval.empty() && strcmp( keyval.c_str(), "0" ) )
        {
                return true;
        }

        return false;
}
// =====================================================================================
//  ParseBrush
//      parse a brush from script
// =====================================================================================
static void ParseBrush( entity_t* mapent, CKeyValues *kv )
{
        brush_t*        b;
        int             i, j;
        side_t*         side;
        contents_t      contents;
        bool            ok;
        bool nullify = CheckForInvisible( mapent );
        hlassume( g_nummapbrushes < MAX_MAP_BRUSHES, assume_MAX_MAP_BRUSHES );

        b = &g_mapbrushes[g_nummapbrushes];
        g_nummapbrushes++;
        b->firstside = g_numbrushsides;
        b->originalentitynum = g_numparsedentities;
        b->originalbrushnum = g_numparsedbrushes;
        b->entitynum = g_bspdata->numentities - 1;
        b->brushnum = g_nummapbrushes - mapent->firstbrush - 1;
        b->bevel = false;
        {
                b->detaillevel = IntForKey( mapent, "zhlt_detaillevel" );
                b->chopdown = IntForKey( mapent, "zhlt_chopdown" );
                b->chopup = IntForKey( mapent, "zhlt_chopup" );
                b->coplanarpriority = IntForKey( mapent, "zhlt_coplanarpriority" );
                bool wrong = false;
                if ( b->detaillevel < 0 )
                {
                        wrong = true;
                        b->detaillevel = 0;
                }
                if ( b->chopdown < 0 )
                {
                        wrong = true;
                        b->chopdown = 0;
                }
                if ( b->chopup < 0 )
                {
                        wrong = true;
                        b->chopup = 0;
                }
                if ( wrong )
                {
                        Warning( "Entity %i, Brush %i: incorrect settings for detail brush.",
                                 b->originalentitynum, b->originalbrushnum
                        );
                }
        }
        for ( int h = 0; h < NUM_HULLS; h++ )
        {
                char key[16];
                const char *value;
                sprintf( key, "zhlt_hull%d", h );
                value = ValueForKey( mapent, key );
                if ( *value )
                {
                        b->hullshapes[h] = _strdup( value );
                }
                else
                {
                        b->hullshapes[h] = NULL;
                }
        }

        mapent->numbrushes++;

        g_TXcommand = 0;

        size_t num_children = kv->get_num_children();
        for (size_t i = 0; i < num_children; i++)
        {
                CKeyValues *child = kv->get_child(i);
                const std::string &name = child->get_name();

                if (name == "side")
                {
                        hlassume( g_numbrushsides < MAX_MAP_SIDES, assume_MAX_MAP_SIDES );
                        side = &g_brushsides[g_numbrushsides];
                        g_numbrushsides++;

                        b->numsides++;
                        side->brushnum = b->brushnum;

                        side->bevel = false;

                        // read the three point plane definition
                        LPoint3 pt1, pt2, pt3;
                        CKeyValues::parse_plane_points(child->get_value("plane"), pt1, pt2, pt3);
                        VectorCopy(pt1, side->planepts[0]);
                        VectorCopy(pt2, side->planepts[1]);
                        VectorCopy(pt3, side->planepts[2]);

                        // Read the material
                        std::string material = child->get_value("material");
                        const char *cmaterial = material.c_str();

                        const BSPMaterial *mat = BSPMaterial::get_from_file( material );
                        if ( !mat )
                        {
                                Error( "Material %s not found!", cmaterial );
                        }
                        if ( g_tex_contents.find( material ) == g_tex_contents.end() )
                        {
                                SetTextureContents( cmaterial, mat->get_contents().c_str() );
                        }

                        {
                                if ( !strncasecmp( cmaterial, "BEVELBRUSH", 10 ) )
                                {
                                        material = "NULL";
                                        b->bevel = true;
                                }
                                if ( !strncasecmp( cmaterial, "BEVEL", 5 ) )
                                {
                                        material = "NULL";
                                        side->bevel = true;
                                }
                                if ( GetTextureContents( cmaterial ) == CONTENTS_NULL )
                                {
                                        material = "NULL";
                                }
                        }
                        cmaterial = material.c_str();
                        safe_strncpy( side->td.name, cmaterial, sizeof( side->td.name ) );

                        // Read material axes
                        LVector3 axis;
                        LVector2 shift_scale;

                        // U axis
                        CKeyValues::parse_material_axis(child->get_value("uaxis"), axis, shift_scale);
                        VectorCopy(axis, side->td.vects.valve.UAxis);
                        side->td.vects.valve.shift[0] = shift_scale[0];
                        side->td.vects.valve.scale[0] = shift_scale[1];

                        // V axis
                        CKeyValues::parse_material_axis(child->get_value("vaxis"), axis, shift_scale);
                        VectorCopy(axis, side->td.vects.valve.VAxis);
                        side->td.vects.valve.shift[1] = shift_scale[0];
                        side->td.vects.valve.scale[1] = shift_scale[1];

                        // Rotation is implicit in U/V axes
                        side->td.vects.valve.rotate = 0;

                        // Lightmap scale
                        side->td.vects.valve.lightmap_scale = atof(child->get_value("lightmap_scale").c_str());
                        std::cout << side->td.vects.valve.lightmap_scale << std::endl;

                        side->td.txcommand = 0;                  // Quark stuff, but needs setting always
                }
        }

        b->contents = contents = CheckBrushContents( b );
        for ( j = 0; j < b->numsides; j++ )
        {
                side = &g_brushsides[b->firstside + j];
                if ( nullify && strncasecmp( side->td.name, "BEVEL", 5 ) && strncasecmp( side->td.name, "ORIGIN", 6 )
                     && strncasecmp( side->td.name, "HINT", 4 ) && strncasecmp( side->td.name, "SKIP", 4 )
                     && strncasecmp( side->td.name, "SOLIDHINT", 9 )
                     && strncasecmp( side->td.name, "SPLITFACE", 9 )
                     && strncasecmp( side->td.name, "BOUNDINGBOX", 11 )
                     && strncasecmp( side->td.name, "CONTENT", 7 ) && strncasecmp( side->td.name, "SKY", 3 )
                     )
                {
                        safe_strncpy( side->td.name, "NULL", sizeof( side->td.name ) );
                }
        }
        for ( j = 0; j < b->numsides; j++ )
        {
                // change to SKIP now that we have set brush content.
                side = &g_brushsides[b->firstside + j];
                if ( !strncasecmp( side->td.name, "SPLITFACE", 9 ) )
                {
                        strcpy( side->td.name, "SKIP" );
                }
        }
        for ( j = 0; j < b->numsides; j++ )
        {
                side = &g_brushsides[b->firstside + j];
                if ( !strncasecmp( side->td.name, "CONTENT", 7 ) )
                {
                        strcpy( side->td.name, "NULL" );
                }
        }
        if ( g_nullifytrigger )
        {
                for ( j = 0; j < b->numsides; j++ )
                {
                        side = &g_brushsides[b->firstside + j];
                        if ( !strncasecmp( side->td.name, "AAATRIGGER", 10 ) )
                        {
                                strcpy( side->td.name, "NULL" );
                        }
                }
        }

        //
        // origin brushes are removed, but they set
        // the rotation origin for the rest of the brushes
        // in the entity
        //

        if ( contents == CONTENTS_ORIGIN )
        {
                if ( *ValueForKey( mapent, "origin" ) )
                {
                        Error( "Entity %i, Brush %i: Only one ORIGIN brush allowed.",
                               b->originalentitynum, b->originalbrushnum
                        );
                }
                char            string[MAXTOKEN];
                vec3_t          origin;

                b->contents = CONTENTS_SOLID;
                CreateBrush( mapent->firstbrush + b->brushnum );     // to get sizes
                b->contents = contents;

                for ( i = 0; i < NUM_HULLS; i++ )
                {
                        b->hulls[i].faces = NULL;
                }

                if ( b->entitynum != 0 )  // Ignore for WORLD (code elsewhere enforces no ORIGIN in world message)
                {
                        VectorAdd( b->hulls[0].bounds.m_Mins, b->hulls[0].bounds.m_Maxs, origin );
                        VectorScale( origin, 0.5, origin );

                        safe_snprintf( string, MAXTOKEN, "%i %i %i", (int)origin[0], (int)origin[1], (int)origin[2] );
                        SetKeyValue( &g_bspdata->entities[b->entitynum], "origin", string );
                }
        }
        if ( *ValueForKey( &g_bspdata->entities[b->entitynum], "zhlt_usemodel" ) )
        {
                memset( &g_brushsides[b->firstside], 0, b->numsides * sizeof( side_t ) );
                g_numbrushsides -= b->numsides;
                for ( int h = 0; h < NUM_HULLS; h++ )
                {
                        if ( b->hullshapes[h] )
                        {
                                free( b->hullshapes[h] );
                        }
                }
                memset( b, 0, sizeof( brush_t ) );
                g_nummapbrushes--;
                mapent->numbrushes--;
                return;
        }
        if ( !strcmp( ValueForKey( &g_bspdata->entities[b->entitynum], "classname" ), "info_hullshape" ) )
        {
                // all brushes should be erased, but not now.
                return;
        }
        if ( contents == CONTENTS_BOUNDINGBOX )
        {
                if ( *ValueForKey( mapent, "zhlt_minsmaxs" ) )
                {
                        Error( "Entity %i, Brush %i: Only one BoundingBox brush allowed.",
                               b->originalentitynum, b->originalbrushnum
                        );
                }
                char            string[MAXTOKEN];
                vec3_t          mins, maxs;
                char			*origin = NULL;
                if ( *ValueForKey( mapent, "origin" ) )
                {
                        origin = strdup( ValueForKey( mapent, "origin" ) );
                        SetKeyValue( mapent, "origin", "" );
                }

                b->contents = CONTENTS_SOLID;
                CreateBrush( mapent->firstbrush + b->brushnum );     // to get sizes
                b->contents = contents;

                for ( i = 0; i < NUM_HULLS; i++ )
                {
                        b->hulls[i].faces = NULL;
                }

                if ( b->entitynum != 0 )  // Ignore for WORLD (code elsewhere enforces no ORIGIN in world message)
                {
                        VectorCopy( b->hulls[0].bounds.m_Mins, mins );
                        VectorCopy( b->hulls[0].bounds.m_Maxs, maxs );

                        safe_snprintf( string, MAXTOKEN, "%.0f %.0f %.0f %.0f %.0f %.0f", mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2] );
                        SetKeyValue( &g_bspdata->entities[b->entitynum], "zhlt_minsmaxs", string );
                }

                if ( origin )
                {
                        SetKeyValue( mapent, "origin", origin );
                        free( origin );
                }
        }

}


// =====================================================================================
//  ParseMapEntity
//      parse an entity from script
// =====================================================================================
bool ParseMapEntity(CKeyValues *kv)
{
        int             this_entity;
        entity_t*       mapent;
        epair_t*        e;

        g_numparsedbrushes = 0;

        this_entity = g_bspdata->numentities;

        hlassume( g_bspdata->numentities < MAX_MAP_ENTITIES, assume_MAX_MAP_ENTITIES );
        g_bspdata->numentities++;

        mapent = &g_bspdata->entities[this_entity];
        mapent->firstbrush = g_nummapbrushes;
        mapent->numbrushes = 0;

        size_t keycount = kv->get_num_keys();
        for (size_t i = 0; i < keycount; i++)
        {
                e = ParseEpair(kv, i);
                if ( mapent->numbrushes > 0 ) Warning( "Error: ParseEntity: Keyvalue comes after brushes." ); //--vluzacn

                if ( !strcmp( e->key, "mapversion" ) )
                {
                        g_nMapFileVersion = atoi( e->value );
                }

                SetKeyValue( mapent, e->key, e->value );
                Free( e->key );
                Free( e->value );
                Free( e );
        }

        size_t count = kv->get_num_children();
        for (size_t i = 0; i < count; i++)
        {
                CKeyValues *child = kv->get_child(i);
                const std::string &name = child->get_name();
                if (name == "solid")
                {
                        ParseBrush(mapent, child);
                        g_numparsedbrushes++;
                }
                else if (name == "connections")
                {
                        size_t keycount = child->get_num_keys();
                        for (size_t j = 0; j < keycount; j++)
                        {
                                e = ParseEpair(child, j);
                                e->next = nullptr;
                                if ( !mapent->epairs )
                                {
                                        mapent->epairs = e;
                                }
                                else
                                {
                                        epair_t *ep;
                                        for ( ep = mapent->epairs; ep->next != nullptr; ep = ep->next )
                                        {
                                        }
                                        ep->next = e;
                                }
                        }
                }
        }

        CheckFatal();
        if ( this_entity == 0 )
        {
                // Let the map tell which version of the compiler it comes from, to help tracing compiler bugs.
                char versionstring[128];
                sprintf( versionstring, "P3BSPTools " TOOLS_VERSIONSTRING " (%s)", __DATE__ );
                SetKeyValue( mapent, "compiler", versionstring );
        }

        if ( !strcmp( ValueForKey( mapent, "classname" ), "info_compile_parameters" ) )
        {
                GetParamsFromEnt( mapent );
        }

	GetVectorForKey( mapent, "origin", mapent->origin );

        if ( !strcmp( "func_group", ValueForKey( mapent, "classname" ) )
             || !strcmp( "func_detail", ValueForKey( mapent, "classname" ) )
             )
        {
                // this is pretty gross, because the brushes are expected to be
                // in linear order for each entity
                brush_t*        temp;
                int             newbrushes;
                int             worldbrushes;
                int             i;

                newbrushes = mapent->numbrushes;
                worldbrushes = g_bspdata->entities[0].numbrushes;

                temp = new brush_t[newbrushes];
                memcpy( temp, g_mapbrushes + mapent->firstbrush, newbrushes * sizeof( brush_t ) );

                for ( i = 0; i < newbrushes; i++ )
                {
                        temp[i].entitynum = 0;
                        temp[i].brushnum += worldbrushes;
                }

                // make space to move the brushes (overlapped copy)
                memmove( g_mapbrushes + worldbrushes + newbrushes,
                         g_mapbrushes + worldbrushes, sizeof( brush_t ) * ( g_nummapbrushes - worldbrushes - newbrushes ) );

                // copy the new brushes down
                memcpy( g_mapbrushes + worldbrushes, temp, sizeof( brush_t ) * newbrushes );

                // fix up indexes
                g_bspdata->numentities--;
                g_bspdata->entities[0].numbrushes += newbrushes;
                for ( i = 1; i < g_bspdata->numentities; i++ )
                {
                        g_bspdata->entities[i].firstbrush += newbrushes;
                }
                memset( mapent, 0, sizeof( *mapent ) );
                delete[] temp;
                return true;
        }

        if ( !strcmp( ValueForKey( mapent, "classname" ), "info_hullshape" ) )
        {
                bool disabled;
                const char *id;
                int defaulthulls;
                disabled = IntForKey( mapent, "disabled" );
                id = ValueForKey( mapent, "targetname" );
                defaulthulls = IntForKey( mapent, "defaulthulls" );
                CreateHullShape( this_entity, disabled, id, defaulthulls );
                DeleteCurrentEntity( mapent );
                return true;
        }
        if ( fabs( mapent->origin[0] ) > ENGINE_ENTITY_RANGE + ON_EPSILON ||
             fabs( mapent->origin[1] ) > ENGINE_ENTITY_RANGE + ON_EPSILON ||
             fabs( mapent->origin[2] ) > ENGINE_ENTITY_RANGE + ON_EPSILON )
        {
                const char *classname = ValueForKey( mapent, "classname" );
                if ( strncmp( classname, "light", 5 ) )
                {
                        Warning( "Entity %i (classname \"%s\"): origin outside +/-%.0f: (%.0f,%.0f,%.0f)",
                                 g_numparsedentities,
                                 classname, (double)ENGINE_ENTITY_RANGE, mapent->origin[0], mapent->origin[1], mapent->origin[2] );
                }
        }

        if ( !strcmp( ValueForKey( mapent, "classname" ), "env_cubemap" ) )
        {
                // consume env_cubemap, emit it to dcubemap_t in bsp file
                dcubemap_t dcm;
                for ( int i = 0; i < 6; i++ )
                {
                        // this will be filled in when you build cubemaps in the engine
                        dcm.imgofs[i] = -1;
                }

                vec3_t origin;
                GetVectorDForKey( mapent, "origin", origin );
                VectorCopy( origin, dcm.pos );

                dcm.size = IntForKey( mapent, "size" );

                g_bspdata->cubemaps.push_back( dcm );

                DeleteCurrentEntity( mapent );
                return true;
        }

        if ( !strcmp( ValueForKey( mapent, "classname" ), "prop_static" ) )
        {
                // static props are consumed and emitted to the BSP file
                dstaticprop_t prop;
                strcpy( prop.name, ValueForKey( mapent, "modelpath" ) );

                vec3_t scale;
                GetVectorDForKey( mapent, "scale", scale );
                VectorCopy( scale, prop.scale );

                vec3_t origin;
                GetVectorDForKey( mapent, "origin", origin );
                VectorCopy( origin, prop.pos );

                vec3_t angles;
                GetVectorDForKey( mapent, "angles", angles );
                VectorCopy( angles, prop.hpr );

                prop.first_vertex_data = -1; // will be filled in by p3rad if STATICPROPFLAGS_STATICLIGHTING bit is set
                prop.num_vertex_datas = 0;
                prop.flags = (unsigned short)IntForKey( mapent, "spawnflags" );
                prop.lightsrc = -1;
                g_bspdata->dstaticprops.push_back( prop );
                propname_to_propnum[ValueForKey( mapent, "targetname" )] = g_bspdata->dstaticprops.size() - 1;
                DeleteCurrentEntity( mapent );
                return true;
        }

        return true;
}

pvector<std::string> GetLightPropSources( const entity_t *ent )
{
        pvector<std::string> result;

        std::string propname = "";
        std::string sources = ValueForKey( ent, "propsources" );
        for ( size_t i = 0; i < sources.size(); i++ )
        {
                char c = sources[i];
                if ( c == ';' )
                {
                        result.push_back( propname );
                        propname = "";
                }
                else
                {
                        propname += c;
                }
        }

        return result;
}

void AssignLightSourcesToProps()
{
        for ( int i = 0; i < g_bspdata->numentities; i++ )
        {
                entity_t *ent = g_bspdata->entities + i;
                const char *classname = ValueForKey( ent, "classname" );
                if ( !strncmp( classname, "light", 5 ) &&
                     strncmp( classname, "light_environment", 18 ) )
                {
                        pvector<std::string> propsources = GetLightPropSources( ent );
                        for ( size_t j = 0; j < propsources.size(); j++ )
                        {
                                std::string src = propsources[j];
                                if ( !src.size() )
                                {
                                        continue;
                                }

                                if ( propname_to_propnum.find( src ) != propname_to_propnum.end() )
                                {
                                        int propnum = propname_to_propnum[src];
                                        g_bspdata->dstaticprops[propnum].lightsrc = i;
                                        Log( "Assigned light %i to prop %i with name %s", i, propnum, src.c_str() );
                                }
                        }
                }
        }
}

// =====================================================================================
//  CountEngineEntities
// =====================================================================================
unsigned int    CountEngineEntities()
{
        unsigned int x;
        unsigned num_engine_entities = 0;
        entity_t*       mapent = g_bspdata->entities;

        // for each entity in the map
        for ( x = 0; x<g_bspdata->numentities; x++, mapent++ )
        {
                const char* classname = ValueForKey( mapent, "classname" );

                // if its a light_spot or light_env, dont include it as an engine entity!
                if ( classname )
                {
                        if ( !strncasecmp( classname, "light", 5 )
                             || !strncasecmp( classname, "light_spot", 10 )
                             || !strncasecmp( classname, "light_environment", 17 )
                             )
                        {
                                const char* style = ValueForKey( mapent, "style" );
                                const char* targetname = ValueForKey( mapent, "targetname" );

                                // lightspots and lightenviroments dont have a targetname or style
                                if ( !strlen( targetname ) && !atoi( style ) )
                                {
                                        continue;
                                }
                        }
                }

                num_engine_entities++;
        }

        return num_engine_entities;
}

// =====================================================================================
//  LoadMapFile
//      wrapper for LoadScriptFile
//      parse in script entities
// =====================================================================================
const char*     ContentsToString( const contents_t type );

void            LoadMapFile( const char* const filename )
{
        unsigned num_engine_entities;

        PT( CKeyValues ) root = CKeyValues::load( Filename::from_os_specific( filename ) );
        if ( !root )
        {
                Error("Couldn't load map file %s", filename);
        }

        CPT( TransformState ) net_transform = TransformState::make_identity();

        g_bspdata->numentities = 0;

        g_numparsedentities = 0;

        size_t count = root->get_num_children();
        for (size_t i = 0; i < count; i++)
        {
                CKeyValues *kv = root->get_child(i);
                const std::string &name = kv->get_name();
                if (name == "world" || name == "entity")
                {
                        ParseMapEntity(kv);
                        g_numparsedentities++;
                }
        }

        AssignLightSourcesToProps();

        // AJM debug
        /*
        for (int i = 0; i < g_numentities; i++)
        {
        Log("entity: %i - %i brushes - %s\n", i, g_entities[i].numbrushes, ValueForKey(&g_entities[i], "classname"));
        }
        Log("total entities: %i\ntotal brushes: %i\n\n", g_numentities, g_nummapbrushes);

        for (i = g_entities[0].firstbrush; i < g_entities[0].firstbrush + g_entities[0].numbrushes; i++)
        {
        Log("worldspawn brush %i: contents %s\n", i, ContentsToString((contents_t)g_mapbrushes[i].contents));
        }
        */

        num_engine_entities = CountEngineEntities();

        hlassume( num_engine_entities < MAX_ENGINE_ENTITIES, assume_MAX_ENGINE_ENTITIES );

        CheckFatal();

        Verbose( "Load map:%s\n", filename );
        Verbose( "%5i brushes\n", g_nummapbrushes );
        Verbose( "%5i map entities \n", g_bspdata->numentities - num_engine_entities );
        Verbose( "%5i engine entities\n", num_engine_entities );

        // AJM: added in
}

import bpy, sys, os, mathutils

argv = sys.argv[sys.argv.index("--") + 1:]
outdir, spec = argv[0], argv[1]          # spec: "OutName=Obj1+Obj2|OutName2=Obj3"

def deselect():
    for o in bpy.data.objects:
        o.select_set(False)

for group in spec.split("|"):
    out_name, obj_names = group.split("=")
    names = obj_names.split("+")
    objs = [bpy.data.objects.get(n) for n in names]
    objs = [o for o in objs if o and o.type == 'MESH']
    if not objs:
        print("SKIP", out_name, "no objects"); continue

    deselect()
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]

    # Linked duplicates share mesh data, and transform_apply refuses to touch a multi-user mesh.
    bpy.ops.object.make_single_user(type='SELECTED_OBJECTS', object=True, obdata=True)

    # Bake object transforms into the mesh so Unreal doesn't inherit Blender-side scaling.
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    if len(objs) > 1:
        bpy.ops.object.join()            # a lamp is one prop, not three
    obj = bpy.context.view_layer.objects.active

    # Re-pivot to base centre: XY centred on the bounding box, Z resting on zero. Without this the
    # pivot stays wherever the object sat in the Blender scene, and Unreal places the actor there
    # while the geometry appears metres away.
    corners = [obj.matrix_world @ mathutils.Vector(c) for c in obj.bound_box]
    cx = (min(c.x for c in corners) + max(c.x for c in corners)) / 2.0
    cy = (min(c.y for c in corners) + max(c.y for c in corners)) / 2.0
    minz = min(c.z for c in corners)
    obj.data.transform(mathutils.Matrix.Translation((-cx, -cy, -minz)))
    obj.data.update()

    deselect()
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(outdir, out_name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, object_types={'MESH'},
                             apply_unit_scale=True, global_scale=1.0,
                             axis_forward='-Z', axis_up='Y', mesh_smooth_type='FACE',
                             path_mode='COPY', embed_textures=True)
    dims = tuple(round(v, 2) for v in obj.dimensions)
    print("EXPORTED %-22s dims=%s -> %s" % (out_name, dims, os.path.basename(path)))

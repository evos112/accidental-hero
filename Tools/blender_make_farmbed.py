"""Generates a raised planting bed for BP_FarmPlot and exports it as FBX.

Run headless:
  blender --background --python Tools/blender_make_farmbed.py -- <out.fbx>

The bed is 2 x 2 m to match the plot's existing 200 cm footprint and its 90 cm slot spacing, with
the pivot at base centre so it drops straight onto terrain. Two material slots: soil and timber.
"""
import bpy, sys, math

argv = sys.argv[sys.argv.index("--") + 1:]
out_path = argv[0]

# Empty the default scene.
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
for m in list(bpy.data.meshes):
    bpy.data.meshes.remove(m)
for m in list(bpy.data.materials):
    bpy.data.materials.remove(m)

mat_soil = bpy.data.materials.new("FarmBed_Soil")
mat_wood = bpy.data.materials.new("FarmBed_Timber")

HALF = 1.0          # 2 m across
RAIL_T = 0.07       # rail half-thickness
RAIL_H = 0.11       # rail half-height
SOIL_H = 0.05       # soil slab half-height

parts = []

def box(name, sx, sy, sz, x, y, z):
    bpy.ops.mesh.primitive_cube_add(size=2.0, location=(x, y, z))
    o = bpy.context.active_object
    o.name = name
    o.scale = (sx, sy, sz)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    parts.append(o)
    return o

# Soil slab, sitting inside the rails.
soil = box("Soil", HALF - RAIL_T, HALF - RAIL_T, SOIL_H, 0, 0, SOIL_H)

# Tilled ridges: five rows running along X, which is what makes it read as worked ground rather
# than a brown box.
ridges = []
for i in range(5):
    y = -0.72 + i * 0.36
    ridges.append(box("Ridge_%d" % i, HALF - RAIL_T - 0.06, 0.105, 0.035, 0, y, SOIL_H * 2 + 0.03))

# Timber edging.
rails = [box("Rail_YN", HALF, RAIL_T, RAIL_H, 0, -HALF + RAIL_T, RAIL_H),
         box("Rail_YP", HALF, RAIL_T, RAIL_H, 0,  HALF - RAIL_T, RAIL_H),
         box("Rail_XN", RAIL_T, HALF - RAIL_T * 2, RAIL_H, -HALF + RAIL_T, 0, RAIL_H),
         box("Rail_XP", RAIL_T, HALF - RAIL_T * 2, RAIL_H,  HALF - RAIL_T, 0, RAIL_H)]

# Corner posts, slightly proud, so the frame reads as built rather than extruded.
posts = []
for sx in (-1, 1):
    for sy in (-1, 1):
        posts.append(box("Post_%d_%d" % (sx, sy), 0.09, 0.09, RAIL_H + 0.03,
                         sx * (HALF - 0.09), sy * (HALF - 0.09), RAIL_H + 0.03))

soil_group = [soil] + ridges
wood_group = rails + posts

bpy.ops.object.select_all(action='DESELECT')
for o in parts:
    o.select_set(True)
bpy.context.view_layer.objects.active = soil
bpy.ops.object.join()
bed = bpy.context.active_object
bed.name = "SM_FarmBed"

# Assign slot 0 = soil, slot 1 = timber, by polygon centre height/position.
bed.data.materials.clear()
bed.data.materials.append(mat_soil)
bed.data.materials.append(mat_wood)
for poly in bed.data.polygons:
    c = poly.center
    inside = abs(c.x) < (HALF - RAIL_T - 0.001) and abs(c.y) < (HALF - RAIL_T - 0.001)
    poly.material_index = 0 if inside else 1

bpy.ops.object.select_all(action='DESELECT')
bed.select_set(True)
bpy.context.view_layer.objects.active = bed
bpy.ops.export_scene.fbx(filepath=out_path, use_selection=True, object_types={'MESH'},
                         apply_unit_scale=True, global_scale=1.0,
                         axis_forward='-Z', axis_up='Y', mesh_smooth_type='FACE')
print("EXPORTED", out_path, "dims", tuple(round(v, 3) for v in bed.dimensions),
      "polys", len(bed.data.polygons))

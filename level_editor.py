import bpy
import math
import json
import bpy_extras
import gpu
import gpu_extras.batch
import copy
import mathutils

# ブレンダーに登録するアドオン情報
bl_info = {
    "name": "レベルエディタ",
    "author": "Taro Kamata",
    "version": (1, 1),
    "blender": (3, 3, 1),
    "location": "",
    "description": "レベルエディタ（コライダー描画・回転スケール対応版）",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Object"
}

# --- コライダー描画クラス ---
class DrawCollider:
    # 描画ハンドルを保持する静的変数
    handle = None

    @staticmethod
    def draw_collider():
        # 頂点データとインデックスデータの初期化
        vertices = {"pos": []}
        indices = []
        
        # 立方体の頂点ローカル座標
        offsets = [
            [-0.5, -0.5, -0.5], [+0.5, -0.5, -0.5],
            [-0.5, +0.5, -0.5], [+0.5, +0.5, -0.5],
            [-0.5, -0.5, +0.5], [+0.5, -0.5, +0.5],
            [-0.5, +0.5, +0.5], [+0.5, +0.5, +0.5]
        ]

        # シーン内の全オブジェクトを走査
        for obj in bpy.context.scene.objects:
            # "collider" プロパティがあり、値が "BOX" の場合のみ処理する
            if "collider" in obj and obj["collider"] == "BOX":
                # 追加前の頂点数を記録
                start = len(vertices["pos"])
                
                # 【追加】カスタムプロパティから Center と Size を取得（なければデフォルト値）
                center = mathutils.Vector(obj.get("collider_center", (0.0, 0.0, 0.0)))
                size = mathutils.Vector(obj.get("collider_size", (2.0, 2.0, 2.0)))
                
                # 各オブジェクトのワールド行列を使用して座標を計算 (回転・スケール対応)
                for offset in offsets:
                    # 1. ローカル空間でのオフセットを計算 (Centerを加味し、Sizeでスケール)
                    local_pos = center + mathutils.Vector((
                        offset[0] * size[0], 
                        offset[1] * size[1], 
                        offset[2] * size[2]
                    ))
                    # 2. オブジェクトのmatrix_worldを掛けてワールド座標に変換
                    world_pos = obj.matrix_world @ local_pos
                    vertices["pos"].append(world_pos)

                # Boxを構成する12本の辺のインデックスを追加
                indices.extend([
                    [start+0, start+1], [start+2, start+3], [start+0, start+2], [start+1, start+3],
                    [start+4, start+5], [start+6, start+7], [start+4, start+6], [start+5, start+7],
                    [start+0, start+4], [start+1, start+5], [start+2, start+6], [start+3, start+7]
                ])

        # 描画する対象がない場合はエラーを防ぐため処理を抜ける
        if not vertices["pos"]:
            return

        # シェーダーの準備と描画
        shader = gpu.shader.from_builtin("UNIFORM_COLOR")
        batch = gpu_extras.batch.batch_for_shader(shader, "LINES", vertices, indices=indices)
        
        # 【変更点】資料に合わせて緑色に変更
        color = [0.0, 1.0, 0.0, 1.0] # 緑色
        shader.bind()
        shader.uniform_float("color", color)
        batch.draw(shader)


# --- 既存のメニュークラス ---
class TOPBAR_MT_my_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_my_menu"
    bl_label = "MyMenu"
    bl_description = "拡張メニュー by " + bl_info["author"]

    def draw(self, context):
        layout = self.layout
        layout.operator(MYADDON_OT_stretch_vertex.bl_idname, text=MYADDON_OT_stretch_vertex.bl_label)
        layout.operator(MYADDON_OT_create_ico_sphere.bl_idname, text=MYADDON_OT_create_ico_sphere.bl_label)
        layout.separator() 
        layout.operator(MYADDON_OT_export_scene.bl_idname, text=MYADDON_OT_export_scene.bl_label)

    def submenu(self, context):
        self.layout.menu(TOPBAR_MT_my_menu.bl_idname)

# --- 既存のオペレータ：頂点を伸ばす ---
class MYADDON_OT_stretch_vertex(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_stretch_vertex"
    bl_label = "頂点を伸ばす"
    bl_description = "頂点座標を引っ張って伸ばします"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        if "Cube" in bpy.data.objects:
            bpy.data.objects["Cube"].data.vertices[0].co.x += 1.0
            return {'FINISHED'}
        else:
            self.report({'WARNING'}, "Cubeが見つかりません")
            return {'CANCELLED'}

# --- 既存のオペレータ：ICO球生成 ---
class MYADDON_OT_create_ico_sphere(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_create_object"
    bl_label = "ICO球生成"
    bl_description = "ICO球を生成します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        bpy.ops.mesh.primitive_ico_sphere_add()
        return {'FINISHED'}

# --- 既存のオペレータ：シーン出力 ---
class MYADDON_OT_export_scene(bpy.types.Operator, bpy_extras.io_utils.ExportHelper):
    bl_idname = "myaddon.myaddon_ot_export_scene"
    bl_label = "シーン出力"
    bl_description = "シーン情報をExportします"

    filename_ext = ".json"

    def write_and_print(self, file, str):
        print(str)
        file.write(str + '\n')

    def parse_scene_recursive(self, file, object, level):
        indent = ''
        for i in range(level):
            indent += "\t"

        self.write_and_print(file, indent + object.type + " - " + object.name)
        
        trans, rot, scale = object.matrix_local.decompose()
        rot = rot.to_euler()
        rot.x, rot.y, rot.z = [math.degrees(v) for v in rot]

        self.write_and_print(file, indent + "Trans(%f,%f,%f)" % (trans.x, trans.y, trans.z))
        self.write_and_print(file, indent + "Rot(%f,%f,%f)" % (rot.x, rot.y, rot.z))
        self.write_and_print(file, indent + "Scale(%f,%f,%f)" % (scale.x, scale.y, scale.z))

        # --- 【追加】ファイル名出力 ---
        if "file_name" in object:
            self.write_and_print(file, indent + "FileName(%s)" % object["file_name"])

        # --- 【追加】コライダー情報の出力 ---
        if "collider" in object:
            self.write_and_print(file, indent + "Collider(%s)" % object["collider"])
            
            center = object.get("collider_center", (0.0, 0.0, 0.0))
            self.write_and_print(file, indent + "ColliderCenter(%f,%f,%f)" % (center[0], center[1], center[2]))
            
            size = object.get("collider_size", (2.0, 2.0, 2.0))
            self.write_and_print(file, indent + "ColliderSize(%f,%f,%f)" % (size[0], size[1], size[2]))

        self.write_and_print(file, '')

        for child in object.children:
            self.parse_scene_recursive(file, child, level + 1)

    def export(self):
        print("シーン情報出力開始... %r" % self.filepath)
        with open(self.filepath, "wt") as file:
            self.write_and_print(file, "SCENE")
            for object in bpy.context.scene.objects:
                if object.parent is None:
                    self.parse_scene_recursive(file, object, 0)

    def parse_scene_recursive_json(self, data_parent, object, level):
        """オブジェクト1個分の情報をJSON用の辞書へ再帰的に格納する。"""
        json_object = dict()

        # オブジェクトの基本情報
        json_object["type"] = object.type
        json_object["name"] = object.name

        # ローカルトランスフォームを取得
        trans, rot, scale = object.matrix_local.decompose()
        rot = rot.to_euler()
        rot.x = math.degrees(rot.x)
        rot.y = math.degrees(rot.y)
        rot.z = math.degrees(rot.z)

        transform = dict()
        transform["translation"] = (trans.x, trans.y, trans.z)
        transform["rotation"] = (rot.x, rot.y, rot.z)
        transform["scaling"] = (scale.x, scale.y, scale.z)
        json_object["transform"] = transform

        # カスタムプロパティ「file_name」
        if "file_name" in object:
            json_object["file_name"] = object["file_name"]

        # カスタムプロパティ「collider」
        if "collider" in object:
            collider = dict()
            collider["type"] = object["collider"]
            collider["center"] = list(
                object.get("collider_center", (0.0, 0.0, 0.0))
            )
            collider["size"] = list(
                object.get("collider_size", (2.0, 2.0, 2.0))
            )
            json_object["collider"] = collider

        # 作成したオブジェクト情報を親のリストへ登録
        data_parent.append(json_object)

        # 子オブジェクトを再帰的に格納
        if len(object.children) > 0:
            json_object["children"] = list()
            for child in object.children:
                self.parse_scene_recursive_json(
                    json_object["children"], child, level + 1
                )

    def export_json(self):
        """シーン情報をJSON形式でファイルへ出力する。"""
        print("JSONシーン情報出力開始... %r" % self.filepath)

        json_object_root = dict()
        json_object_root["name"] = "scene"
        json_object_root["objects"] = list()

        # 親を持たないオブジェクトから再帰走査を開始する
        for object in bpy.context.scene.objects:
            if object.parent:
                continue
            self.parse_scene_recursive_json(
                json_object_root["objects"], object, 0
            )

        # インデント付きのJSON文字列へ変換
        json_text = json.dumps(
            json_object_root,
            ensure_ascii=False,
            cls=json.JSONEncoder,
            indent=4
        )
        print(json_text)

        # UTF-8でJSONファイルへ書き出す
        with open(self.filepath, "wt", encoding="utf-8") as file:
            file.write(json_text)

    def execute(self, context):
        print("シーン情報をExportします")
        self.export_json()
        self.report({'INFO'}, "シーン情報をExportしました")
        return {'FINISHED'}

# --- 既存のオペレータ：カスタムプロパティ追加 ---
class MYADDON_OT_add_filename(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_add_filename"
    bl_label = "FileName 追加"
    bl_description = "['file_name']カスタムプロパティを追加します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        context.object["file_name"] = ""
        return {'FINISHED'}

# --- 【新規追加】オペレータ：コライダー追加 ---
class MYADDON_OT_add_collider(bpy.types.Operator):
    bl_idname = "myaddon.add_collider"
    bl_label = "コライダー追加"
    bl_description = "オブジェクトにカスタムプロパティとしてコライダーを追加します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        obj = context.active_object
        if obj:
            # プロパティの初期値を設定
            obj["collider"] = "BOX"
            obj["collider_center"] = mathutils.Vector((0.0, 0.0, 0.0))
            obj["collider_size"] = mathutils.Vector((2.0, 2.0, 2.0))
        return {'FINISHED'}


# --- 既存のパネルクラス ---
class OBJECT_PT_file_name(bpy.types.Panel):
    bl_idname = "OBJECT_PT_file_name"
    bl_label = "FileName"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"

    def draw(self, context):
        layout = self.layout
        obj = context.object

        layout.operator(MYADDON_OT_stretch_vertex.bl_idname, text=MYADDON_OT_stretch_vertex.bl_label)
        layout.operator(MYADDON_OT_create_ico_sphere.bl_idname, text=MYADDON_OT_create_ico_sphere.bl_label)
        layout.operator(MYADDON_OT_export_scene.bl_idname, text=MYADDON_OT_export_scene.bl_label)

        layout.separator()

        if "file_name" in obj:
            layout.prop(obj, '["file_name"]', text="FileName:")
        else:
            layout.operator(MYADDON_OT_add_filename.bl_idname)

# --- パネルクラス：コライダーUI ---
class OBJECT_PT_collider(bpy.types.Panel):
    bl_label = "Collider"
    bl_idname = "OBJECT_PT_collider"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "object"

    def draw(self, context):
        layout = self.layout
        obj = context.active_object
        if not obj:
            return

        # プロパティがある場合はType, Center, Sizeを表示
        if "collider" in obj:
            layout.prop(obj, '["collider"]', text="Type")
            layout.prop(obj, '["collider_center"]', text="Center")
            layout.prop(obj, '["collider_size"]', text="Size")
        # ない場合は「コライダー追加」ボタンを表示
        else:
            layout.operator(MYADDON_OT_add_collider.bl_idname, text="コライダー追加")

# --- 登録・解除処理 ---
classes = (
    MYADDON_OT_stretch_vertex,
    MYADDON_OT_create_ico_sphere,
    MYADDON_OT_export_scene,
    MYADDON_OT_add_filename,
    MYADDON_OT_add_collider,
    TOPBAR_MT_my_menu,
    OBJECT_PT_file_name,
    OBJECT_PT_collider,
)

def register():
    # クラスの登録
    for cls in classes:
        bpy.utils.register_class(cls)
    
    # メニューの追加
    bpy.types.TOPBAR_MT_editor_menus.append(TOPBAR_MT_my_menu.submenu)
    
    # 描画ハンドルの登録
    DrawCollider.handle = bpy.types.SpaceView3D.draw_handler_add(
        DrawCollider.draw_collider, (), "WINDOW", "POST_VIEW"
    )
    
    print("レベルエディタが有効化されました。")

def unregister():
    # 描画ハンドルの解除
    if DrawCollider.handle is not None:
        bpy.types.SpaceView3D.draw_handler_remove(DrawCollider.handle, "WINDOW")
        DrawCollider.handle = None

    # メニューの削除
    bpy.types.TOPBAR_MT_editor_menus.remove(TOPBAR_MT_my_menu.submenu)
    
    # クラスの登録解除（逆順）
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
        
    print("レベルエディタが無効化されました。")

if __name__ == "__main__":
    register()

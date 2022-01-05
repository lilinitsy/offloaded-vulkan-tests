use std::{
    ffi::{CStr, CString},
    fmt::Display,
    os::raw::c_char,
    process::exit,
    slice,
};
use tobj::LoadOptions;

// We handle most errors by killing the process, to avoid the pain that is converting a Rust panic
// to a C++ exception.
fn die<T: Display>(msg: &str, err: T) -> ! {
    eprintln!("{}: {}", msg, err);
    exit(1)
}

macro_rules! make_extern_vec {
    ($name:ident, $ty:ty) => {
        #[repr(C)]
        struct $name {
            ptr: *mut $ty,
            len: usize,
        }

        impl Drop for $name {
            fn drop(&mut self) {
                unsafe { drop(Box::from_raw(slice::from_raw_parts_mut(self.ptr, self.len))) }
            }
        }

        impl From<Box<[$ty]>> for $name {
            fn from(xs: Box<[$ty]>) -> $name {
                let len = xs.len();
                let ptr = Box::into_raw(xs) as *mut _;
                $name { ptr, len }
            }
        }

        impl From<Vec<$ty>> for $name {
            fn from(xs: Vec<$ty>) -> Self {
                $name::from(xs.into_boxed_slice())
            }
        }

        impl Into<Box<[$ty]>> for $name {
            fn into(self) -> Box<[$ty]> {
                unsafe { Box::from_raw(slice::from_raw_parts_mut(self.ptr, self.len)) }
            }
        }
    };
}

make_extern_vec!(F32s, f32);
make_extern_vec!(U32s, u32);

#[repr(C)]
struct Model {
    positions: F32s,
    normals: F32s,
    texcoords: F32s,
    indices: U32s,
    material_id: usize,
    has_material: u8,
    name: *mut c_char,
}

impl Drop for Model {
    fn drop(&mut self) {
        todo!()
    }
}

impl From<tobj::Model> for Model {
    fn from(model: tobj::Model) -> Self {
        assert!(model.mesh.vertex_color.is_empty());
        assert!(model.mesh.face_arities.is_empty());
        assert!(model.mesh.texcoord_indices.is_empty());
        assert!(model.mesh.normal_indices.is_empty());

        Model {
            positions: model.mesh.positions.into(),
            normals: model.mesh.normals.into(),
            texcoords: model.mesh.texcoords.into(),
            indices: model.mesh.indices.into(),
            material_id: model.mesh.material_id.unwrap_or_default(),
            has_material: if model.mesh.material_id.is_some() {
                1
            } else {
                0
            },
            name: CString::new(model.name)
                .unwrap_or_else(|err| die("failed to convert model name to CString", err))
                .into_raw(),
        }
    }
}

make_extern_vec!(Models, Model);

#[repr(C)]
pub struct Material {
    // Add other properties as they're needed.
    diffuse_texture: *mut c_char,
}

impl From<tobj::Material> for Material {
    fn from(material: tobj::Material) -> Material {
        Material {
            diffuse_texture: CString::new(material.diffuse_texture)
                .unwrap_or_else(|err| die("failed to convert texture path to CString", err))
                .into_raw(),
        }
    }
}

make_extern_vec!(Materials, Material);

#[repr(C)]
pub struct ModelsAndMaterials {
    models: Models,
    materials: Materials,
}

#[no_mangle]
pub extern "C" fn load_obj(path: *const c_char) -> ModelsAndMaterials {
    let path = unsafe {
        CStr::from_ptr(path)
            .to_str()
            .unwrap_or_else(|err| die("failed to convert path to String", err))
    };

    let result = tobj::load_obj(
        path,
        &LoadOptions {
            single_index: true,
            triangulate: true,
            ignore_points: true,
            ignore_lines: true,
        },
    );
    let (models, materials_result) =
        result.unwrap_or_else(|err| die("Failed to load OBJ file", err));
    let materials =
        materials_result.unwrap_or_else(|err| die("Failed to load materials from OBJ file", err));

    let models = models.into_iter().map(Model::from).collect::<Vec<_>>();
    let materials = materials
        .into_iter()
        .map(Material::from)
        .collect::<Vec<_>>();

    ModelsAndMaterials {
        models: Models::from(models),
        materials: Materials::from(materials),
    }
}

import os

dxc = "..\\libs\\dxc\\bin\\dxc.exe"
shader_dir = "..\\Src\\"
output_dir = "..\\Src\\CompiledShaders\\"
profile = "lib_6_3"
additional_args = " -Zpr "

raygen_shader_list = {
	"Raygen.hlsl" : ".rgs" 
}

mtl_shader_list = {
	"ClosestHit.hlsl" : ".chs", 
	"Miss.hlsl" : ".miss"
}

def build_raygen_shaders():
	bytecode_prefix = "raygen"
	for shader_file, bytecode_annotation in raygen_shader_list.items():
		shader_path = shader_dir + shader_file
		compiled_bytecode_path = output_dir + bytecode_prefix + bytecode_annotation + ".cso"
		os.system(dxc + " -T" + profile + " -Fo" + compiled_bytecode_path + additional_args + shader_path)

def build_default_shaders():
	bytecode_prefix = "mtl_default"
	for shader_file, bytecode_annotation in mtl_shader_list.items():
		shader_path = shader_dir + shader_file
		compiled_bytecode_path = output_dir + bytecode_prefix + bytecode_annotation + ".cso"
		os.system(dxc + " -T" + profile + " -Fo" + compiled_bytecode_path + additional_args + shader_path)

def build_masked_shaders():
	bytecode_prefix = "mtl_masked"
	for shader_file, bytecode_annotation in mtl_shader_list.items():
		shader_path = shader_dir + shader_file
		compiled_bytecode_path = output_dir + bytecode_prefix + bytecode_annotation + ".cso"
		os.system(dxc + " -T" + profile + " -Fo" + compiled_bytecode_path + additional_args + shader_path)

def build_untextured_shaders():
	bytecode_prefix = "mtl_untextured"
	for shader_file, bytecode_annotation in mtl_shader_list.items():
		shader_path = shader_dir + shader_file
		compiled_bytecode_path = output_dir + bytecode_prefix + bytecode_annotation + ".cso"
		os.system(dxc + " -T" + profile + " -Fo" + compiled_bytecode_path + additional_args + shader_path)


build_raygen_shaders()
build_default_shaders()
build_masked_shaders()
build_untextured_shaders()
print("Shader compilation finished")

#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import subprocess

# `scons p=linux` to compile for linux
# `scons p=windows` to compile for windows

# on linux you can optionally use `use_llvm=yes` to use clang instead of gcc

project_name = "vulkan_tutorial"
output_folder = "bin/"

# TODO Platform-independent Vulkan SDK path
vulkan_sdk_dir = "C:/VulkanSDK/1.1.82.1/"

#------------------------------------------------------------------------------
env = Environment()

if ARGUMENTS.get('use_llvm', 'no') == 'yes':
	env['CXX'] = 'clang++'

target = ARGUMENTS.get('target', 'release')
platform = ARGUMENTS.get('p', 'windows')

#------------------------------------------------------------------------------
if ARGUMENTS.get('compile_shaders', 'no') == 'yes':

	def compile_shaders():
		# TODO Platform-independent shader compilation
		compiler_path = os.path.join(vulkan_sdk_dir, "Bin32/glslangValidator.exe")
		shaders = [
			"shaders/default.frag",
			"shaders/default.vert"
		]
		for shader in shaders:
			print("Compiling shader ", shader)
			target_path = os.path.join(output_folder, os.path.basename(shader) + '.spv')
			subprocess.call([compiler_path, '-V', shader, '-o', target_path])

	compile_shaders()

#------------------------------------------------------------------------------
#env['TARGET_ARCH'] = 'x86_64'

env.Append(LIBS = [
	"vulkan-1",
	"glfw3dll"
])
env.Append(CPPPATH = [
	"thirdparty/glfw/include",
	"."
])
env.Append(CPPDEFINES = [
	"GLFW_DLL"
])

if platform == 'linux':

	env.Append(CCFLAGS = ['-g','-O3', '-std=c++14'])
	env.Append(LINKFLAGS = ['-Wl,-R,\'$$ORIGIN\''])
	# TODO Linux setup

elif platform == "osx":

	platform_dir = 'osx'
	env.Append(CCFLAGS = ['-g','-O3', '-arch', 'x86_64'])
	env.Append(LINKFLAGS = ['-arch', 'x86_64'])
	# TODO OSX setup

elif platform == 'windows':

	env.Append(LIBPATH = [
		"thirdparty/glfw/lib-vc2015",
		os.path.join(vulkan_sdk_dir, "Lib")
	])
	env.Append(CPPPATH = [
		os.path.join(vulkan_sdk_dir, "Include")
	])
	# This is needed if you statically link GLFW
	# env.Append(LIBS = [
	# 	"user32",
	# 	"gdi32",
	# 	"shell32"
	# ])

	if target == 'debug':
		env.Append(CPPDEFINES = ['DEBUG', '_DEBUG'])
		env.Append(CCFLAGS='/MDd') # TODO Use MD in release, `d` means it's debug
		env.Append(CCFLAGS=Split('/Zi /Fd${TARGET}.pdb'))
		env.Append(LINKFLAGS = ['/DEBUG'])

	else:
		# TODO Windows release config
		pass

#------------------------------------------------------------------------------
sources = []

def add_sources(sources, dir):
	for f in os.listdir(dir):
		if f.endswith('.cpp') or f.endswith('.c'):
			sources.append(dir + '/' + f)

add_sources(sources, 'core')
add_sources(sources, 'core/math')
add_sources(sources, 'game')

#------------------------------------------------------------------------------
program = env.Program(target=(output_folder + project_name), source=sources)
Default(program)



#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

# `scons p=linux` to compile for linux
# `scons p=windows` to compile for windows

# on linux you can optionally use `use_llvm=yes` to use clang instead of gcc

project_name = "vulkan_tutorial"
output_folder = "bin/"

#------------------------------------------------------------------------------
env = Environment()

if ARGUMENTS.get('use_llvm', 'no') == 'yes':
	env['CXX'] = 'clang++'

target = ARGUMENTS.get('target', 'release')
platform = ARGUMENTS.get('p', 'windows')

#------------------------------------------------------------------------------
#env['TARGET_ARCH'] = 'x86_64'

env.Append(LIBS = [
	"vulkan-1",
	"glfw3dll"
])
env.Append(CPPPATH = [
	"thirdparty/glfw/include",
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
		"C:/VulkanSDK/1.1.82.1/Lib"
	])
	env.Append(CPPPATH = [
		# TODO Independent Vulkan SDK path (embed in project?)
		"C:/VulkanSDK/1.1.82.1/Include"
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

add_sources(sources, '.')

#------------------------------------------------------------------------------
program = env.Program(target=(output_folder + project_name), source=sources)
Default(program)



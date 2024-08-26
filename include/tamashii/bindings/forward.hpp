#pragma once

#define T_USE_NANOBIND_LITERALS using namespace nb::literals;

#define T_BEGIN_PYTHON_NAMESPACE namespace tamashii::python {
#define T_END_PYTHON_NAMESPACE }
#define T_USE_PYTHON_NAMESPACE using namespace tamashii; \
							   using namespace tamashii::python;


T_BEGIN_PYTHON_NAMESPACE

class CoreModule;

class CcliVar;
class Exports;

class Model;

class Light;
class PointLight;
class DirectionalLight;

class Camera;

class Scene;

T_END_PYTHON_NAMESPACE

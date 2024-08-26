#pragma once
#include <tamashii/core/defines.h>

struct ImGuiIO;

T_BEGIN_NAMESPACE

namespace io {
	struct SceneData;
}

class Asset;
class Node;
class Mesh;
class Model;
class Camera;
class Material;
class Image;
class Light;
class IESLight;
class ImageBasedLight;
struct Texture;
struct TRS;


struct Ref;
struct RefLight;
struct RefModel;
struct RefMesh;
struct RefCamera;
struct RefCameraPrivate;


struct ViewDef_s;
struct Ref;
struct UiConf_s;
struct ScreenshotInfo_s;
struct DrawInfo;
struct Intersection;


class Window;
class RenderScene;
class RenderSystem;
class RenderBackendImplementation;
class RenderBackend;

T_END_NAMESPACE
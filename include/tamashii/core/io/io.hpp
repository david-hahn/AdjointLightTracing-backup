#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/forward.h>

#include <filesystem>
#include <deque>
#include <memory>

T_BEGIN_NAMESPACE

namespace io {
	struct SceneData
	{
		float mCycleTime;
		std::deque<std::shared_ptr<Node>> mSceneGraphs;
		std::deque<std::shared_ptr<Model>> mModels;
		std::deque<std::shared_ptr<Camera>> mCameras;
		std::deque<std::shared_ptr<Light>> mLights;
		std::deque<Material*> mMaterials;
		std::deque<Texture*> mTextures;
		std::deque<Image*> mImages;

		SceneData() : mCycleTime{ 0.0f } {}
		[[nodiscard]] static std::unique_ptr<SceneData> alloc() { return std::make_unique<SceneData>(); }
	};

	class Import {
	public:
		using ImportFunction = std::function<std::unique_ptr<SceneData>(const std::string&)>;

		static Import& instance()
		{
			static Import instance;
			return instance;
		}
		Import(Import const&) = delete;
		void								operator=(Import const&) = delete;

		/* SCENE */
		
		[[nodiscard]] std::unique_ptr<SceneData>	load_scene(const std::string& aFile) const;

		void add_load_scene_format(const std::string& aName, const std::vector<std::string>& aExt, const ImportFunction& aCallback)
		{
			mSceneLoadFormats.emplace_back(aName, aExt, aCallback);
		}

		[[nodiscard]] std::vector<std::pair<std::string, std::vector<std::string>>> load_scene_file_dialog_info() const;

		/* MODEL */
		[[nodiscard]] static std::unique_ptr<Model>	load_model(const std::string& aFile);
		[[nodiscard]] static std::unique_ptr<Mesh>	load_mesh(const std::string& aFile);

		/* LIGHT */
		[[nodiscard]] static std::unique_ptr<Light>	load_light(const std::filesystem::path& aFile);

		/* IMAGE */
		[[nodiscard]] static Image* load_image_8_bit(std::string const& aFile, int aForceNumberOfChannels = 0);
		[[nodiscard]] static Image* load_image_16_bit(std::string const& aFile, int aForceNumberOfChannels = 0);

		/* FILE */
		
		
		static std::string					load_file(std::string const& aFilename);

		/* Loader Backend */
		/* SCENE */
		
		[[nodiscard]] static std::unique_ptr<SceneData>	load_gltf(std::string const& aFile);
		
		[[nodiscard]] static std::unique_ptr<SceneData>	load_pbrt(std::string const& aFile);
		[[nodiscard]] static std::unique_ptr<SceneData>	load_bsp(std::string const& aFile);

		/* MODEL */
		
		[[nodiscard]] static std::unique_ptr<Model>	load_ply(const std::string& aFile);
		[[nodiscard]] static std::unique_ptr<Mesh>	load_ply_mesh(const std::string& aFile);
		
		[[nodiscard]] static std::unique_ptr<Model>	load_obj(const std::string& aFile);
		[[nodiscard]] static std::unique_ptr<Mesh>	load_obj_mesh(const std::string& aFile);

		/* LIGHT */
		
		[[nodiscard]] static std::unique_ptr<IESLight>		load_ies(const std::filesystem::path& aFile);
		
		[[nodiscard]] static std::unique_ptr<Light>			load_ldt(const std::filesystem::path& aFile);

	private:
		Import() : mSceneLoadFormats{
		{ "glTF (.gltf/.glb)" , { "*.gltf", "*.glb" }, &load_gltf },
		{ "pbrt-v3 (.pbrt)" , { "*.pbrt" },&load_pbrt },
		{ "Quake III (.bsp)" , { "*.bsp" },&load_bsp }
		} {}

		std::deque<std::tuple<std::string, std::vector<std::string>, ImportFunction>> mSceneLoadFormats;
	};

	class Export {
	public:
		struct SceneExportSettings
		{
			
			enum class Format : uint32_t
			{
				glTF = 0x1
			};
			Format mFormat;
			bool mEmbedImages;
			bool mEmbedBuffers;
			bool mWriteBinary;
			bool mExcludeLights;
			bool mExcludeModels;
			bool mExcludeCameras;
			SceneExportSettings() : mFormat(Format::glTF), mEmbedImages(false), mEmbedBuffers(false),
				mWriteBinary(false), mExcludeLights(false), mExcludeModels(false), mExcludeCameras(false) {}

			SceneExportSettings(uint32_t aEncoded) : mFormat(static_cast<Format>(aEncoded & 0xf)), mEmbedImages(aEncoded & 0x10),
				mEmbedBuffers(aEncoded & 0x20), mWriteBinary(aEncoded & 0x40),
				mExcludeLights(aEncoded & 0x80), mExcludeModels(aEncoded & 0x100), mExcludeCameras(aEncoded & 0x200) {}
			uint32_t encode() const
			{
				return static_cast<uint32_t>(mFormat)
					| static_cast<uint32_t>(mEmbedImages << 4)
					| static_cast<uint32_t>(mEmbedBuffers << 5)
					| static_cast<uint32_t>(mWriteBinary << 6)
					| static_cast<uint32_t>(mExcludeLights << 7)
					| static_cast<uint32_t>(mExcludeModels << 8)
					| static_cast<uint32_t>(mExcludeCameras << 9);
			}
		};
		static bool			save_scene(const std::string& aOutputFile, const SceneExportSettings aSettings, const SceneData& aSceneInfo)
		{
			std::string ext = std::filesystem::path(aOutputFile).extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
			if (ext == ".gltf" || ext == ".glb") { 
				return save_scene_gltf(aOutputFile, aSettings, aSceneInfo);
			}
			spdlog::warn("...format not supported");
			return false;
		}
		/* IMAGE */
		static void			save_image_png_8_bit(std::string const& aName, int aWidth, int aHeight, int aChannels, const uint8_t* aPixels);
		
		
		static void			save_image_exr(std::string const& aName, int aWidth, int aHeight, const float* aPixels, uint32_t aStride = 3, const std::vector<uint8_t>& aOut = { 2, 1, 0 } /*B G R*/);
		/* FILE */
		static bool			write_file(std::string const& aFilename, std::string const& aContent);
		static bool			write_file_append(std::string const& aFilename, std::string const& aContent);

		static bool			save_scene_gltf(const std::string& aOutputFile, SceneExportSettings aSettings, const SceneData& aSceneInfo);
	};
}

T_END_NAMESPACE
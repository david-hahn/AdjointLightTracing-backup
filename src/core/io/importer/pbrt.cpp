#include <tamashii/core/io/io.hpp>
#include <tamashii/core/scene/camera.hpp>
#include <tamashii/core/scene/image.hpp>
#include <tamashii/core/scene/material.hpp>
#include <tamashii/core/scene/model.hpp>
#include <tamashii/core/scene/scene_graph.hpp>
#include <tamashii/core/io/io.hpp>
#include <tamashii/core/common/math.hpp>
#include <fstream>
#include <stack>
#include <filesystem>


T_USE_NAMESPACE
namespace {

	std::string getNextWord(std::string& s) {
		std::string::size_type p1 = 0;
		std::string::size_type p2 = 0;
		const std::string delimiters_front = " \n\t\"[]";
		std::string delimitersBack = " \"]";

		p1 = s.find_first_not_of(delimiters_front, p1);
		
		const std::string::size_type temp = s.find_first_of('\"');
		if (temp != std::string::npos && p1 == (temp + 1)) {
			delimitersBack.erase(std::remove(delimitersBack.begin(), delimitersBack.end(), ' '), delimitersBack.end());
		}
		if (p1 == std::string::npos) return "";
		p2 = s.find_first_of(delimitersBack, p1);
		if (p2 == std::string::npos) p2 = s.size();
		std::string re = s.substr(p1, (p2 - p1));
		if (s.size() == p2) {
			s = "";
			return re;
		}
		s = s.substr(p2+1, (s.size() - p2));
		return re;
	}
}

std::unique_ptr<io::SceneData> io::Import::load_pbrt(std::string const& aFile) {
	std::ifstream f(aFile);
	std::string path = std::filesystem::path(aFile).parent_path().string();

	std::unique_ptr<SceneData> scene = SceneData::alloc();
	std::shared_ptr tscene = Node::alloc("pbrt scene");
	Node& rootNode = tscene->addChildNode("root");
	Node& cameraNode = rootNode.addChildNode("camera");
	Node& geometryNode = rootNode.addChildNode("geometry");
	std::stack<glm::mat4> transforms; 
	transforms.push(glm::mat4(1.0f));

	std::string line;
	float value;
	int pos;
	
	while (f.good()) {
		std::getline(f, line);
		const std::string lineType = getNextWord(line);
		if (lineType == "Transform") {
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					transforms.top()[i][j] = std::stof(getNextWord(line));
				}
			}
			transforms.top() = glm::transpose(transforms.top());
			transforms.top()[2] *= glm::vec4(-1);
			transforms.top() = glm::transpose(transforms.top());
		}
		if (lineType == "Camera") {
			bool cam_type_perspective = false;
			float fov;
			std::string arg;
			while (!(arg = getNextWord(line)).empty()) {
				if (arg == "perspective") cam_type_perspective = true;
				else if (arg == "float fov") fov = std::stof(getNextWord(line));
			}

			std::shared_ptr<Camera> cam{ Camera::alloc() };
			cam->setName("Camera");
			if(cam_type_perspective) cam->initPerspectiveCamera(glm::radians(fov), 1.0f, 0.1f, 10000.0f);

			Node& node = cameraNode.addChildNode("node");
			node.setCamera(cam);
			geometryNode.setModelMatrix(transforms.top());
		}
		if (lineType == "WorldBegin") {
			transforms.pop();
			transforms.push(glm::dmat4(1.0));
			break;
		}
	}

	std::shared_ptr<Model> tmodel;
	Material* tmaterial = nullptr;
	Light* tlight = nullptr;
	std::map<std::string, Texture*> textureMap;
	std::map<std::string, Material*> materialMap;
	
	bool skip = false;
	bool attribute = false;
	auto color = glm::vec4(0);
	while (f.good()) {
		std::getline(f, line);

		const std::string line_type = getNextWord(line);

		if (line_type == "TransformBegin") {
			transforms.push(glm::dmat4(1.0));
		}
		else if (line_type == "TransformEnd") {
			transforms.pop();
		}
		else if (line_type == "Transform") {
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					transforms.top()[i][j] = std::stof(getNextWord(line));
				}
			}
			
		}

		else if (line_type == "AttributeBegin") {
			transforms.push(glm::dmat4(1.0));
			skip = true;
			attribute = true;
		}
		else if (line_type == "AttributeEnd") {
			transforms.pop();
			color = glm::vec4(0);
			skip = false;
			attribute = false;
		}
		

		else if (line_type == "AreaLightSource") {
			/*tlight = new SurfaceLight();
			scene.lights.push_back(tlight);
			Node* Node = Node::alloc("light node");
			Node->setLight(tlight);
			tRootNode->addChildNode(Node);*/
			std::string arg;
			while (!(arg = getNextWord(line)).empty()) {
				if (arg == "rgb L") {
					color.x = std::stof(getNextWord(line));
					color.y = std::stof(getNextWord(line));
					color.z = std::stof(getNextWord(line));
					color.w = 1;
					
				}
			}
		}

		else if (line_type == "Texture") {
			const std::string texture_name = getNextWord(line);
			getNextWord(line);
			getNextWord(line);
			const std::string texture_type = getNextWord(line);
			if (texture_type == "string filename") {
				const std::string texture_filepath = std::filesystem::path(path + "/" + getNextWord(line)).make_preferred().string();
				spdlog::info("Load Image: {}", texture_filepath);
				Image* img = load_image_8_bit(texture_filepath, 4);
				img->needsMipMaps(true);
				scene->mImages.push_back(img);

				Texture* tex = Texture::alloc();
				tex->sampler = { Sampler::Filter::LINEAR, Sampler::Filter::LINEAR, Sampler::Filter::LINEAR,
					Sampler::Wrap::REPEAT, Sampler::Wrap::REPEAT, Sampler::Wrap::REPEAT, 0, std::numeric_limits<float>::max() };
				tex->texCoordIndex = 0;
				tex->image = img;
				textureMap.insert({ texture_name, tex });
				scene->mTextures.push_back(tex);
			}
		}
		else if (line_type == "MakeNamedMaterial") {
			const std::string material_name = getNextWord(line);
			spdlog::info("Load Material: {}", material_name);

			Material* mat = Material::alloc(material_name);
			scene->mMaterials.push_back(mat);
			materialMap.insert({ material_name, mat });

			bool uber = false;
			std::string arg;
			while (!(arg = getNextWord(line)).empty()) {
				if (arg == "string type") {
					std::string type = getNextWord(line);
					if (type == "mirror") {
						mat->setMetallicFactor(1);
						mat->setRoughnessFactor(0);
						uber = false;
					}
					else if (type == "metal") {
						mat->setMetallicFactor(1);
						mat->setRoughnessFactor(0.01f);
						uber = false;
					}
					else if (type == "glass") {
						mat->setMetallicFactor(0);
						mat->setRoughnessFactor(0);
						mat->setTransmissionFactor(1);
						uber = false;
					}
					else if (type == "uber") {
						mat->setMetallicFactor(0);
						mat->setRoughnessFactor(0.1f);
						mat->setTransmissionFactor(0);
						uber = true;
					}
					else if (type == "matt") {
						mat->setMetallicFactor(0);
						mat->setRoughnessFactor(1);
						uber = false;
					}
					else if (type == "substrate") {
						mat->setMetallicFactor(0);
						mat->setRoughnessFactor(1);
						uber = false;
					} else uber = false;
				}
				else if (arg == "texture Kd") {
					Texture* tex = textureMap[getNextWord(line)];
					tex->image->setSRGB(true);
					mat->setBaseColorTexture(tex);
				} else if (arg == "texture opacity") {
					Texture* tex = textureMap[getNextWord(line)];
					for(uint8_t &c : tex->image->getDataVector()) c = 255 - c;
					mat->setTransmissionTexture(tex);
					mat->setTransmissionFactor(1);
				} else if (arg == "rgb Kd") {
					glm::vec4 c;
					c.x = std::stof(getNextWord(line));
					c.y = std::stof(getNextWord(line));
					c.z = std::stof(getNextWord(line));
					c.w = 1;
					mat->setBaseColorFactor(c);
				}
				else if (arg == "rgb Ks") {
					glm::vec3 c;
					c.x = std::stof(getNextWord(line));
					c.y = std::stof(getNextWord(line));
					c.z = std::stof(getNextWord(line));
					mat->setSpecularColorFactor(c);
				}
				else if (arg == "rgb Ks") {
				}
				else if (arg == "rgb opacity") {
					glm::vec3 o;
					o.x = std::stof(getNextWord(line));
					o.y = std::stof(getNextWord(line));
					o.z = std::stof(getNextWord(line));
					mat->setTransmissionFactor(1.0f - glm::compMax(o));
				}
				else if (arg == "float uroughness") {
					mat->setRoughnessFactor(std::stof(getNextWord(line)));
				}
				else if (arg == "eta") {
					mat->setIOR(std::stof(getNextWord(line)));
				}
			}
			if (uber) mat->setBaseColorFactor(glm::vec4(1.0f));
		}
		else if (line_type == "NamedMaterial") {
			tmaterial = materialMap[getNextWord(line)];
			/*if (tmodel) {
				scene.models.push_back(tmodel);
				Node* Node = Node::alloc("node");
				Node->setModel(tmodel);
				tRootNode->addChildNode(Node);
				tmodel = nullptr;
			}

			tmodel = modelManager.alloc("");
			tmodel->setIndex(modelManager.size());
			modelManager.add(tmodel);
			tmaterial = material_map[getNextWord(line)];*/
		}
		else if (line_type == "Shape") {
			const std::string shape_type = getNextWord(line);
			if (shape_type == "plymesh") {
				const std::string mesh_type = getNextWord(line);
				if (mesh_type == "string filename") {
					const std::filesystem::path mesh_filepath = std::filesystem::path(path + "/" + getNextWord(line)).make_preferred();
					std::shared_ptr<Mesh> tmesh = Import::load_mesh(mesh_filepath.string());
					if(tmaterial) tmesh->setMaterial(tmaterial);
					tmodel = Model::alloc(mesh_filepath.filename().string());
					scene->mModels.push_back(tmodel);
					Node& node = geometryNode.addChildNode("node");
					node.setModel(tmodel);
					

					aabb_s aabb = tmodel->getAABB();
					aabb.set(tmesh->getAABB());
					tmodel->setAABB(aabb);
					tmodel->addMesh(tmesh);
					tmodel = nullptr;
				}
			}
			else if (shape_type == "trianglemesh") {
				std::vector<uint32_t> indices;
				std::vector<float> vertices;
				std::vector<float> normals;
				std::vector<float> uvs;

				std::shared_ptr<Mesh> tmesh = Mesh::alloc();
				tmesh->setTopology(Mesh::Topology::TRIANGLE_LIST);

				std::string arg;
				while (!(arg = getNextWord(line)).empty()) {
					if (arg == "integer indices") {
						tmesh->hasIndices(true);
						while (!(arg = getNextWord(line)).empty()) {
							try {
								indices.push_back(static_cast<uint32_t>(std::stof(arg)));
							}
							catch (const std::exception&) {
								break;
							}
						}
					}
					if (arg == "point P") {
						while (!(arg = getNextWord(line)).empty()) {
							try {
								vertices.push_back({ std::stof(arg) });
							}
							catch (const std::exception&) {
								break;
							}
						}
					}
					if (arg == "normal N") {
						tmesh->hasNormals(true);
						while (!(arg = getNextWord(line)).empty()) {
							try {
								normals.push_back({ std::stof(arg) });
							}
							catch (const std::exception&) {
								break;
							}
						}
					}
					if (arg == "float uv") {
						tmesh->hasTexCoords0(true);
						while (!(arg = getNextWord(line)).empty()) {
							try {
								uvs.push_back({ std::stof(arg) });
							}
							catch (const std::exception&) {
								break;
							}
						}
					}
				}
				const size_t vertexCount = vertices.size() / 3;

				std::vector<vertex_s> v;
				v.reserve(vertexCount);

				aabb_s aabb;
				for (size_t i = 0; i < vertexCount; i++) {
					aabb.set(reinterpret_cast<glm::vec3*>(vertices.data())[i]);
					v.push_back({ glm::vec4(reinterpret_cast<glm::vec3*>(vertices.data())[i],1),
								glm::vec4(reinterpret_cast<glm::vec3*>(normals.data())[i],0),
								glm::vec4(0),
								glm::vec2(reinterpret_cast<glm::vec2*>(uvs.data())[i])});
				}
				tmesh->setAABB(aabb);

				
				tmesh->setIndices(indices);
				tmesh->setVertices(v);

				if (tmaterial) {
					Material* mat = Material::alloc("temp");
					scene->mMaterials.push_back(mat);
					*mat = *tmaterial;
                    glm::vec3 vec = color;
                    mat->setEmissionFactor(vec);
					tmesh->setMaterial(mat);
				}

				tmodel = Model::alloc("");
				scene->mModels.push_back(tmodel);

				glm::vec3 scale;
				glm::quat rotation;
				glm::vec3 translation;
				ASSERT(math::decomposeTransform(transforms.top(), translation, rotation, scale), "decompose error");

				Node& node = geometryNode.addChildNode("node");
				node.setModel(tmodel);
				node.setRotation(glm::vec4(rotation.x, rotation.y, rotation.z, rotation.w));
				node.setScale(glm::vec3(scale.x, scale.y, scale.z));
				node.setTranslation(glm::vec3(translation.x, translation.y, translation.z));

				aabb.set(tmesh->getAABB());
				tmodel->setAABB(aabb);
				tmodel->addMesh(tmesh);
				tmodel = nullptr;
			}
			else if (shape_type == "sphere") {
				
				
				
				
				

				
				
				
				
				

				
				
				
				
				
				

				
				
				
				
				
				
			}
		}

		else if (line_type == "WorldEnd") {
			if (tmodel) {
				scene->mModels.push_back(tmodel);
				Node& node = geometryNode.addChildNode("node");
				node.setModel(tmodel);
				tmodel = nullptr;
			}
			break;
		}
	}
	scene->mSceneGraphs.push_back(tscene);
	return scene;
}

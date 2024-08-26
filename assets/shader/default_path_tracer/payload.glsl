
struct RayPayload {
	vec2 hitAttribute;
	int instanceID;
	int customIndex;
	int primitiveID;
    int geometryIndex;
	uint hitKind;
	float hit_distance;
};
struct ShadowRayPayload {
	float visible;
};






        traceRayEXT(tlas, rayDesc.flags, 0xff, 0, 0, 0, rayDesc.origin, rayDesc.tmin, rayDesc.direction, rayDesc.tmax, 0);
        if ( /*MISS*/ ap.instanceIndex == -1) break;
        hd.ray = rayDesc;

        /* GATHER GEO DATA */
        hd.bary = computeBarycentric(ap.hitAttribute);
        const GeometryData_s geometry_data = geometry_buffer[ap.customInstanceID + ap.geometryIndex];
        const Material_s material = material_buffer[geometry_data.material_index];
        
        
        hd.idx[0] = (ap.primitiveIndex * 3) + 0;
        hd.idx[1] = (ap.primitiveIndex * 3) + 1;
        hd.idx[2] = (ap.primitiveIndex * 3) + 2;
        if (geometry_data.has_indices > 0)
        {
            hd.idx[0] = index_buffer[geometry_data.index_buffer_offset + hd.idx[0]];
            hd.idx[1] = index_buffer[geometry_data.index_buffer_offset + hd.idx[1]];
            hd.idx[2] = index_buffer[geometry_data.index_buffer_offset + hd.idx[2]];
        }
        hd.idx[0] += geometry_data.vertex_buffer_offset;
        hd.idx[1] += geometry_data.vertex_buffer_offset;
        hd.idx[2] += geometry_data.vertex_buffer_offset;
        
        hd.radBuffIdx[0] = hd.idx[0] * ENTRIES_PER_VERTEX;
        hd.radBuffIdx[1] = hd.idx[1] * ENTRIES_PER_VERTEX;
        hd.radBuffIdx[2] = hd.idx[2] * ENTRIES_PER_VERTEX;

        
        const mat3 normal_mat = mat3(transpose(inverse(geometry_data.model_matrix)));
        const vec3 n_0 = vertex_buffer[hd.idx[0]].normal.xyz;
        const vec3 n_1 = vertex_buffer[hd.idx[1]].normal.xyz;
        const vec3 n_2 = vertex_buffer[hd.idx[2]].normal.xyz;
        hd.hitN = normalize(normal_mat * (n_0 * hd.bary[0] + n_1 * hd.bary[1] + n_2 * hd.bary[2]));
        
        const vec3 t_0 = vertex_buffer[hd.idx[0]].tangent.xyz;
        const vec3 t_1 = vertex_buffer[hd.idx[1]].tangent.xyz;
        const vec3 t_2 = vertex_buffer[hd.idx[2]].tangent.xyz;
        hd.hitT = normal_mat * (t_0 * hd.bary[0] + t_1 * hd.bary[1] + t_2 * hd.bary[2]); 
        hd.hitT -= dot(hd.hitT, hd.hitN) * hd.hitN; 
        hd.hitT = normalize(hd.hitT); 
        
        const vec3 v0 = (geometry_data.model_matrix * vertex_buffer[hd.idx[0]].position).xyz;
        const vec3 v1 = (geometry_data.model_matrix * vertex_buffer[hd.idx[1]].position).xyz;
        const vec3 v2 = (geometry_data.model_matrix * vertex_buffer[hd.idx[2]].position).xyz;
        hd.hitX = hd.bary[0] * v0 + hd.bary[1] * v1 + hd.bary[2] * v2;
        if (dot(hd.hitN, hd.hitN) < 0.5f) hd.hitN = normalize(cross(v1 - v0, v2 - v0)); 
        if (dot(hd.hitT, hd.hitT) < 0.5f || abs(dot(hd.hitN, hd.hitT)) > 1e-5) hd.hitT = getPerpendicularVector(hd.hitN); 
        

        
        hd.texColor = material.baseColorFactor.xyz;
        hd.texNormal = hd.hitN;
        hd.texMetallic = material.metallicFactor;
        hd.texRoughness = material.roughnessFactor;
        hd.texTransmission = material.transmissionFactor;
        hd.eta_i = 1.0f; hd.eta_o = material.ior;
        {
            const vec2 uv_0 = vertex_buffer[hd.idx[0]].texture_coordinates_0.xy;
            const vec2 uv_1 = vertex_buffer[hd.idx[1]].texture_coordinates_0.xy;
            const vec2 uv_2 = vertex_buffer[hd.idx[2]].texture_coordinates_0.xy;
            const vec2 uv = (hd.bary.x * uv_0 + hd.bary.y * uv_1 + hd.bary.z * uv_2);
            if (material.baseColorTexIdx != -1) {
                hd.texColor *= (texture(texture_sampler[material.baseColorTexIdx], uv)).xyz;
            }
            if (material.metallicTexIdx != -1) {
                hd.texMetallic *= texture(texture_sampler[material.metallicTexIdx], uv).r;
            }
            if (material.roughnessTexIdx != -1) {
                hd.texRoughness *= texture(texture_sampler[material.roughnessTexIdx], uv).r;
            }
            if (material.transmissionTexIdx != -1) {
                hd.texTransmission *= texture(texture_sampler[material.transmissionTexIdx], uv).r;
            }
            if (material.normalTexIdx != -1) {
                hd.texNormal = texture(texture_sampler[material.normalTexIdx], uv).xyz;
                hd.texNormal = nTangentSpaceToWorldSpace(hd.texNormal, material.normalScale, hd.hitT, cross(hd.hitN, hd.hitT), hd.hitN);
            }
        }

        bd.inCos = dot(-rayDesc.direction, hd.hitN);
        if (bd.inCos < 1e-5f) break;

        
        if(handleBounce(hd, bd, lightIndex)) break;

        const uint numSamples = 1 + (6 / bd.bounce);
        const float pdf = PDF_UNIT_HEMISPHERE_UNIFORM;
        bool nextBounce = false;
        vec3 outRadiance;
        [[unroll]]
        for (uint localSample = 0u; localSample < numSamples; localSample++) {
            vec3 wo = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereUniform(randomFloat2(seed)), hd.hitN));
            const float outCos = dot(wo, hd.hitN);
            if (outCos < 1e-5f) continue;
            nextBounce = true;
            bd.wo = wo;
#if USE_GGX
            vec3 brdf = metalroughness_eval(-rayDesc.direction, bd.wo, hd.texNormal, hd.texColor, hd.texMetallic, hd.texRoughness, hd.texTransmission, hd.eta_int, hd.eta_ext, false);
#else
            vec3 brdf = lambert_brdf_eval(bd.wo, hd.texNormal, hd.texColor);
#endif
            outRadiance = brdf * bd.rayThroughput / pdf;
            
            vec3 sampleRadiance = brdf * bd.rayThroughput / outCos / float(numSamples);

            
            if(handleSample(brdf, sampleRadiance, hd, bd, lightIndex)) continue;
        }
        if (bd.bounce == 1)  {
            gd.dBrdfdP /= float(numSamples);
            gd.dRaydP /= float(numSamples);
            gd.dRaydN /= float(numSamples);
            gd.dRaydT /= float(numSamples);
            gd.dRaydAio /= float(numSamples);
        }
        if (pdf < 1e-5f || !nextBounce) break; 
        bd.rayThroughput = outRadiance;
        rayDesc.origin = hd.hitX;
        rayDesc.direction = bd.wo;

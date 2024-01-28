#version 330 core

uniform sampler2D  gPosition;
uniform sampler2D  gNormal;
uniform sampler2D  gAlbedo;
uniform sampler2D depthTexture;
uniform vec3 lightPos;
uniform mat4 projection;
uniform int maxLevel;
in vec2 texCoords;
in vec3 normals;
out vec4 FragColor;

#define HIZ_START_LEVEL 0
#define HIZ_STOP_LEVEL maxLevel
#define MAX_ITERATIONS 50
const float INFINITY = 1.0e38; 


struct CrossStepOffset {
    vec2 crossStep;
    vec2 crossOffset;
};

struct RayHitResult {
    vec3 hitPosition;
    vec3 hitColor;
};

struct RayPositionLevel {
    vec3 rayStart;
    int level;
};

vec3 intersectDepthPlane(vec3 o, vec3 d, float t){
    return o + d * t;
}

//calculates the cell index in a grid for a given position.
vec2 getCell(vec2 pos, vec2 startCellCount) {
    vec2 scaledPos = pos * startCellCount;
    vec2 clampedPos = clamp(scaledPos, vec2(0.0), startCellCount - vec2(1.0));
    return vec2(int(clampedPos.x), int(clampedPos.y)); 
}


vec3 worldToScreenSpace(vec3 worldPos, mat4 projMatrix) {
    vec4 clipPos = projMatrix * vec4(worldPos, 1.0);
    vec3 ndc = clipPos.xyz / clipPos.w;
    return (ndc + vec3(1.0)) * 0.5;  // Transform NDC to screen space coordinates in the range [0, 1]
}

vec3 intersectCellBoundary(vec3 origin, vec3 direction, vec2 cellIndex, vec2 cellCount, vec2 cellStep, vec2 offset) {
    // Calculate the actual size of the cell
    vec2 cellSize = 1.0 / cellCount;
    
    // Calculate the target UV for the next cell boundary
    vec2 nextCellBoundaryUV = (cellIndex + cellStep) * cellSize + offset * cellStep;

    // Calculate the time 't' required for the ray to reach the next cell boundary from the origin
    vec2 t = (nextCellBoundaryUV - origin.xy) / direction.xy;

    // Avoid division by zero
    if(direction.x != 0.0) {
        t.x = (nextCellBoundaryUV.x - origin.x) / direction.x;
    }
    else {
        t.x = 1e30; // Use a large number to represent no intersection in this direction
    }

    if(direction.y != 0.0) {
        t.y = (nextCellBoundaryUV.y - origin.y) / direction.y;
    }   
    else {
        t.y = 1e30; // Same as above
    }
    // Select the shorter time 't' in the x and y directions
    float tMin = min(t.x, t.y);

    // Use time 'tMin' to calculate the intersection of the ray with the cell boundary
    vec3 intersection = origin + direction * tMin;
    
    return intersection;
}

// Get the step size for a given level
vec2 getStepSize(int level) {
    float size = 1.0 / pow(2.0, float(level));
    return vec2(size);
}

vec2 calculateNextBoundaryUV(vec2 currentUV, vec3 rayDir, int level) {
    vec2 cellStep = sign(rayDir.xy) * getStepSize(level);
    vec2 nextUV = currentUV + cellStep;
    return nextUV;
}

// Get the next boundary depth value at the current ray position from the Hi-Z buffer
float getCellBoundaryDepthPlane(sampler2D hiZTexture, vec2 currentUV, vec3 rayDir, int level) {
    // Determine the intersection of the ray with the next cell boundary of the current level in the Hi-Z texture
    vec2 nextBoundaryUV = calculateNextBoundaryUV(currentUV, rayDir, level);

    // Retrieve the depth value of the next boundary in the given level of the Hi-Z buffer
    float nextBoundaryDepth = textureLod(hiZTexture, nextBoundaryUV, float(level)).r;

    return nextBoundaryDepth;
}



vec3 hizTrace(vec3 p, vec3 v) {
    // Set the maximum level of the hierarchical Z-buffer
    float rootLevel = float(maxLevel) - 1.0f;
    // Start from the initial level of the hierarchical Z-buffer
    float level = float(HIZ_START_LEVEL);

    // Initialize the ray with the original screen coordinates and depth
    vec3 ray = p;

    // Normalize the vector to ensure the maximum depth component is 1.0f
    vec3 d = normalize(v);

    // Set the starting point where the depth component is 0.0f (minimum depth)
    vec3 o = intersectDepthPlane(p, d, -p.z);

    // Iterate through the hierarchical Z-buffer levels
    for (; level >= float(HIZ_STOP_LEVEL); level = min(rootLevel, level)) {
        // Obtain the minimum depth of the current cell in the hierarchical Z-buffer
        float minZ = textureLod(depthTexture, ray.xy, int(level)).r;
        
        // Calculate the cell count of the current level
        vec2 cellCount = textureSize(depthTexture, int(level));
        vec2 oldCellIdx = getCell(ray.xy, cellCount);

        // Calculate the intersection point if the ray's depth is below the minimum depth plane
        vec3 currentRay = intersectDepthPlane(o, d, (minZ - ray.z));

        // Determine the new cell index for the ray
        vec2 newCellIdx = getCell(currentRay.xy, cellCount);

        // Check if the ray has crossed a cell boundary
        if (oldCellIdx != newCellIdx) {
            // Calculate the step size for the current level
            vec2 stepSize = getStepSize(int(level));

            // Calculate the UV coordinates of the next boundary
            vec2 nextBoundaryUV = calculateNextBoundaryUV(ray.xy, d, int(level));

            // Obtain the depth value at the next boundary
            float boundaryDepth = getCellBoundaryDepthPlane(depthTexture, nextBoundaryUV, d, int(level));

            // Update the ray using the boundary depth
            currentRay = intersectDepthPlane(o, d, (boundaryDepth - ray.z));
        }

        ray = currentRay;
    }

    return ray;
}


vec2 signNonZero(vec2 v) {
	return vec2((v.x >= 0.0) ? 1.0 : -1.0, (v.y >= 0.0) ? 1.0 : -1.0);
}


// Calculate the step and offset for crossing a cell based on the ray direction
CrossStepOffset calculateCrossStepAndOffset(vec3 rayDir) {
    CrossStepOffset result;
    result.crossStep = signNonZero(rayDir.xy); // Determine the crossing direction for each axis
    result.crossOffset = vec2(0.00001); // Small offset to avoid numerical instability
    return result;
}

// Update the ray's position and level for hierarchical Z-buffer tracing
RayPositionLevel updateRayPositionAndLevel(vec3 start, vec3 o, vec3 d, float minZ, float deltaZ, float cellMinimumDepth, vec2 crossStep, vec2 crossOffset, int level) {
    // Update the ray start if the depth of the current cell is greater than the ray start
    if (cellMinimumDepth > start.z) {
        start = intersectDepthPlane(o, d, (cellMinimumDepth - minZ) / deltaZ);
    }
    // Calculate the intersection with the boundary of the next cell
    start = intersectCellBoundary(o, d, getCell(start.xy, textureSize(depthTexture, level)), textureSize(depthTexture, level), crossStep, crossOffset);
    // Update the level, moving up to a finer level
    level = min(maxLevel, level + 1);

    RayPositionLevel result;
    result.rayStart = start;
    result.level = level;    
    return result;
}


void initializeRayTraversal(inout vec3 rayStart, vec3 rayDirection, int mipLevel, out vec2 step, out vec2 offset, out float depthRange, out vec3 origin) {
    // Calculate cross step and offset based on ray direction
    CrossStepOffset stepOffset = calculateCrossStepAndOffset(rayDirection);
    step = stepOffset.crossStep;
    offset = stepOffset.crossOffset;

    // Initialize depth range and origin
    depthRange = rayDirection.z; // Assuming rayStart.z + rayDirection.z simplifies to rayDirection.z
    origin = rayStart;

    // Initial cell boundary intersection
    vec2 initialCell = getCell(rayStart.xy, textureSize(depthTexture, mipLevel));
    rayStart = intersectCellBoundary(origin, rayDirection, initialCell, textureSize(depthTexture, mipLevel), step, offset);
}

// Function to find the intersection of a ray with the scene
RayHitResult FindIntersection(vec3 rayStart, vec3 rayDirection) {
    vec2 step, offset;
    float depthRange;
    vec3 origin;
    int mipLevel = HIZ_START_LEVEL; // Initialize the mip level for hierarchical z-buffer traversal
    float depthDirection = (rayDirection.z < 0.0) ? -1.0 : 1.0; // Determine if the ray is moving towards or away from the scene
    float startDepth = rayStart.z; // Store the initial depth of the ray

    // Initialize ray traversal parameters
    initializeRayTraversal(rayStart, rayDirection, mipLevel, step, offset, depthRange, origin);

    for (int i = 0; i < MAX_ITERATIONS; i++) {
        vec2 cellDim = textureSize(depthTexture, mipLevel); // Get dimensions of the current cell
        vec2 currentCell = getCell(rayStart.xy, cellDim); // Determine the current cell in the Hi-Z texture
        float cellDepthMin = textureLod(depthTexture, rayStart.xy, mipLevel).r;
        
        // Handle cases where the ray should traverse through depth planes
        if (cellDepthMin > rayStart.z && depthDirection == 1.0) {
            rayStart = intersectDepthPlane(origin, rayDirection, (cellDepthMin - startDepth) / depthRange);
        }

        vec2 nextCell = getCell(rayStart.xy, cellDim); // Determine the next cell in the Hi-Z texture

        // Update the ray position and mip level based on boundary crossing
        if (currentCell != nextCell) {
            rayStart = intersectCellBoundary(origin, rayDirection, currentCell, cellDim, step, offset);
            mipLevel = min(maxLevel, mipLevel + 1); // Increase mip level if not at the maximum level
        } 
        else {
            mipLevel = max(0, mipLevel - 1); // Decrease mip level if not at the base level
        }
    }

    RayHitResult hitResult;
    hitResult.hitPosition = rayStart; // Store the final ray hit position
    hitResult.hitColor = texture(gAlbedo, rayStart.xy).rgb; // Retrieve the color at the hit position from the albedo texture
    return hitResult; // Return the result of the ray intersection
}






void main() {
    // Retrieve view space position and normal from the G-buffer
    vec3 viewSpacePos = texture(gPosition, texCoords).rgb;
    vec3 viewSpaceNor = texture(gNormal, texCoords).rgb;

    // Compute the view direction and the reflection direction
    vec3 viewDir = normalize(viewSpacePos);
    vec3 reflectDir = normalize(reflect(viewDir, viewSpaceNor));

    // Transform the view and reflected positions to screen space
    vec4 clipPosStart = projection * vec4(viewSpacePos, 1.0);
    vec4 clipPosEnd = projection * vec4(viewSpacePos + reflectDir, 1.0);
    vec3 screenPosStart = (clipPosStart.xyz / clipPosStart.w + vec3(1.0)) * 0.5;
    vec3 screenPosEnd = (clipPosEnd.xyz / clipPosEnd.w + vec3(1.0)) * 0.5;

    // Determine the ray tracing direction
    vec3 rayDir = normalize(screenPosEnd - screenPosStart);
  
    // Perform ray tracing to find the intersection point
    vec3 hitPos = hizTrace(screenPosStart, rayDir);
    RayHitResult hitResult = FindIntersection(screenPosStart, rayDir);

    // Extract color from the texture
    vec3 textureColor = texture(gAlbedo, hitPos.xy).rgb;

    // Calculate the reflected color
    vec3 reflectedColor = hitResult.hitColor * textureColor;

    // Blend the texture color with the reflected color
    float alpha = 0.5;
    vec3 totalColor = mix(textureColor, reflectedColor, alpha);

    FragColor = vec4(totalColor, 1.0);
}




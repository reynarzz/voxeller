#pragma once


enum class TextureType
{
	// Will generate performant meshes, but texture will be more complex and bigger in size. 
	// Perfect for complex meshes. Bake supported.
	Atlas,
    
	//// Will generate a not so performant meshes, and a no so performant texture, but final mesh fidelity will be higher. 
	//// Great for non interactive media (videos/images). Bake supported.
	//AtlasHighFidelity,

	// Will generate meshes with more complex topology but simpler and smaller texture made of individual colors per pixel. 
	// Perfect for simple meshes. Baking is not possible/harder to do.
	Palette
};
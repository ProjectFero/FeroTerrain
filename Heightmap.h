

#pragma once

#include "GameFramework/Actor.h"
#include "Heightmap.generated.h"

/**
 * Heightmap for the QuadTree
 */
UCLASS()
class TERRAINGENERATION_API AHeightmap : public AActor
{
	GENERATED_UCLASS_BODY()

public:

	/** Loads the heightmap */
	void Load(FString heightmapName, int32 width, int32 height, float heightScale);
	
	/** Samples the height at a specefic index */
	float SampleHeight(int32 index);

	/** Samples the height at a specefic index */
	float SampleHeight(float x, float z);

	/** This function loads a RAW file */
	bool LoadRAWElevation(FString heightmapName);

	/** Gets the height at a specefied height */
	float GetHeight(float x, float z);

	/** Checks to see if a valid indice is in bounds */
	bool InBounds(int x, int z);

	/** Smooth out the heightmap */
	void Smooth();

	/** Function computes the average height of the ij element.
	    It averages itself with its eight neighbor pixels.  Note
	    that if a pixel is missing neighbor, we just don't include it
	    in the average--that is, edge pixels don't have a neighbor pixel. */
	float Average(int x, int z);

private:

	/** Value to scale the entire heightmap by */
	float m_HeightScale;

	/** Width and Height of the heightmap */
	int32 m_Width;
	int32 m_Height;

	/** List of the height values */
	TArray<float> m_Heightmap;
};

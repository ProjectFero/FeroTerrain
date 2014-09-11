

#include "TerrainGeneration.h"
#include "Heightmap.h"

#include <fstream>
using namespace std;

AHeightmap::AHeightmap(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	
}


void AHeightmap::Load(FString heightmapName, int32 width, int32 height, float heightScale)
{
	m_Width = width;
	m_Height = height;
	m_HeightScale = heightScale;

	LoadRAWElevation(heightmapName);
}

float AHeightmap::SampleHeight(int32 index)
{
	return m_Heightmap[index];
}

float AHeightmap::SampleHeight(float x, float z)
{
	return m_Heightmap[x + (z * m_Width + 1)];
}

bool AHeightmap::LoadRAWElevation(FString heightmapName)
{
	TArray<unsigned char> in;

	//Open the file
	ifstream inFile(TCHAR_TO_ANSI(*heightmapName), ios_base::binary);

	if (inFile)
	{
		//Read the RAW bytes
		inFile.read((char*)&in[0], (streamsize)in.GetTypeSize());

		//Done with file
		inFile.close();
	}

	//Copy the array data into a float array and scale it.
	in.Reset(m_Width * m_Height);
	m_Heightmap.Init(m_Width * m_Height);

	for (int32 i = 0; i < m_Width * m_Height; ++i)
	{
		m_Heightmap[i] = (in[i] / 255.0f) * m_HeightScale;
	}

	return true;
}

float AHeightmap::GetHeight(float x, float z)
{
	float w = (m_Width - 1) * 0.5f;
	float depth = (m_Height - 1) * 0.5f;

	//Transform from terrain local space to "cell" space
	float c = (x + 0.5f * w) / 0.5f;
	float d = (z - 0.5f * depth) / -0.5f;

	//Get the row and column we are in
	int row = (int)floorf(d);
	int col = (int)floorf(c);

	//Grab the heights of the cell we are in
	// A*--*B
	//  | /|
	//  |/ |
	// C*--*D
	float A = m_Heightmap[row * m_Width + col];
	float B = m_Heightmap[row * m_Width + col + 1];
	float C = m_Heightmap[(row + 1) * m_Height + col];
	float D = m_Heightmap[(row + 1) * m_Height + col + 1];

	//Where we are relative to the cell.
	float s = c - (float)col;
	float t = d - (float)row;

	//If upper triangle ABC.
	if (s + t <= 1.0f)
	{
		float uy = B - A;
		float vy = C - A;

		return A + s * uy + t * vy;
	}
	else//lower triangle DCB
	{
		float uy = C - D;
		float vy = B - D;

		return D - (1.0f - s) * uy + (1.0f - t) * vy;
	}
}

bool AHeightmap::InBounds(int x, int z)
{
	return x >= 0 && x < m_Height && z >= 0 && z < m_Width;
}

void AHeightmap::Smooth()
{
	TArray<float> dest;

	dest.Init(m_Width * m_Height);

	for (int32 i = 0; i < m_Height; ++i)
	{
		for (int32 j = 0; j < m_Width; ++j)
		{
			dest[i * m_Width + j] = Average(i, j);
		}
	}

	m_Heightmap = dest;
}

float AHeightmap::Average(int x, int z)
{
	// ----------
	// | 1| 2| 3|
	// ----------
	// |4 |ij| 6|
	// ----------
	// | 7| 8| 9|
	// ----------

	float avg = 0.0f;
	float num = 0.0f;

	// Use int to allow negatives.  If we use UINT, @ i=0, m=i-1=UINT_MAX
	// and no iterations of the outer for loop occur.
	for (int m = x - 1; m <= x + 1; ++m)
	{
		for (int n = z - 1; n <= z + 1; ++n)
		{
			if (InBounds(m, n))
			{
				avg += m_Heightmap[m * m_Width + n];
				num += 1.0f;
			}
		}
	}

	return avg / num;
}

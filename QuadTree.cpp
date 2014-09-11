

#include "TerrainGeneration.h"
#include "QuadTree.h"


AQuadTree::AQuadTree(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	m_Mesh = PCIP.CreateDefaultSubobject<UGeneratedMeshComponent>(this, TEXT("TerrainMesh"));
	//m_Heightmap = PCIP.CreateDefaultSubobject<AHeightmap>(this, TEXT("Heightmap"));

	//Create a default 128x128 mesh
	/*for (int32 y = 0; y < 8; y++)
	{
		for (int32 x = 0; x < 8; x++)
		{
			m_Vertices.Add(FVector(x, y, 0));
		}
	}*/

	m_Vertices.Add(FVector(20, 5, 0));
	m_Vertices.Add(FVector(15, 6, 0));
	m_Vertices.Add(FVector(12, 7, 0));
	m_Vertices.Add(FVector(11, 8, 0));
	m_Vertices.Add(FVector(8, 7, 0));
	m_Vertices.Add(FVector(7, 6, 0));
	m_Vertices.Add(FVector(4, 5, 0));
	m_Vertices.Add(FVector(3, 4, 0));
	m_Vertices.Add(FVector(2, 3, 0));
	m_Vertices.Add(FVector(1, 4, 0));

	//Load some default values for the quadtree
	m_QuadTreeSize = 64;
	m_LeafWidth = 5;
	m_TerrScale = 8;

	LoadQuadTree();

	//Generate the triangles
	TArray<FGeneratedMeshTriangle> triangles;
	Lathe(m_Vertices, triangles, 128);
	//m_Mesh->SetGeneratedMeshTriangles(triangles);
	m_Mesh->SetGeneratedMeshTriangles(m_Triangles);

	RootComponent = m_Mesh;
}

void AQuadTree::Lathe(const TArray<FVector>& points, TArray<FGeneratedMeshTriangle>& triangles, int32 segments)
{
	UE_LOG(LogClass, Log, TEXT("AQuadTree::Lathe POINTS %d"), points.Num());

	// precompute some trig
	float angle = FMath::DegreesToRadians(360.0f / segments);
	float sinA = FMath::Sin(angle);
	float cosA = FMath::Cos(angle);
	
	
	//This implementation is rotation around the X Axis, other formulas below

	//Z Axis Rotation
	//x' = x*cos q - y*sin q
	//y' = x*sin q + y*cos q
	//z' = z

	//X Axis Rotation
	//y' = y*cos q - z*sin q
	//z' = y*sin q + z*cos q
	//x' = x

	//Y Axis Rotation
	//z' = z*cos q - x*sin q
	//x' = z*sin q + x*cos q
	//y' = y

	//Working point array, in which we keep the rotated line we draw with
	TArray<FVector> wp;
	for (int i = 0; i < points.Num(); i++) {
		wp.Add(points[i]);
	}

	// Add a first and last point on the axis to complete the triangles
	FVector p0(wp[0].X, 0, 0);
	FVector pLast(wp[wp.Num() - 1].X, 0, 0);

	FGeneratedMeshTriangle tri;
	//for each segment draw the triangles clockwise for normals pointing out or counterclockwise for the opposite (this here does CW)
	for (int segment = 0; segment<segments; segment++) {

		for (int i = 0; i<points.Num() - 1; i++) {
			FVector p1 = wp[i];
			FVector p2 = wp[i + 1];
			FVector p1r(p1.X, p1.Y*cosA - p1.Z*sinA, p1.Y*sinA + p1.Z*cosA);
			FVector p2r(p2.X, p2.Y*cosA - p2.Z*sinA, p2.Y*sinA + p2.Z*cosA);

			if (i == 0) {
				tri.Vertex0 = p1;
				tri.Vertex1 = p0;
				tri.Vertex2 = p1r;
				triangles.Add(tri);
			}

			tri.Vertex0 = p1;
			tri.Vertex1 = p1r;
			tri.Vertex2 = p2;
			triangles.Add(tri);

			tri.Vertex0 = p2;
			tri.Vertex1 = p1r;
			tri.Vertex2 = p2r;
			triangles.Add(tri);

			if (i == points.Num() - 2) {
				tri.Vertex0 = p2;
				tri.Vertex1 = p2r;
				tri.Vertex2 = pLast;
				triangles.Add(tri);
				wp[i + 1] = p2r;
			}

			wp[i] = p1r;
		}
	}
}

void AQuadTree::GenerateMesh()
{
	GenerateMesh(m_nodes);
}

void AQuadTree::GenerateMesh(FAQuadNode* node)
{
	if (node == 0) return;

	//Generate the mesh for the node
	LatheMesh(node);

	//Progress through the children
	GenerateMesh(node->m_Children[0]);
	GenerateMesh(node->m_Children[1]);
	GenerateMesh(node->m_Children[2]);
	GenerateMesh(node->m_Children[3]);
}

void AQuadTree::LatheMesh(FAQuadNode* node)
{
	if (node == 0) return;

	if (node->m_Type != LEAF) return;

	FVector bounds[4];

	int terrScale = m_TerrScale;//Nice Big size is 8
	// Center the grid in model space
	float halfWidth = ((float)m_LeafWidth - 1.0f) / 2.0f;
	float halfLength = ((float)m_LeafWidth - 1.0f) / 2.0f;

	bounds[0].X = (node->m_Bounds[0].X - halfWidth) * m_TerrScale;
	bounds[0].Y = (node->m_Bounds[0].Z - halfLength) * m_TerrScale;
	bounds[0].Z = 0;

	bounds[1].X = (node->m_Bounds[1].X - halfWidth) * m_TerrScale;
	bounds[1].Y = (node->m_Bounds[1].Z - halfLength) * m_TerrScale;
	bounds[1].Z = 0;

	bounds[2].X = (node->m_Bounds[2].X - halfWidth) * m_TerrScale;
	bounds[2].Y = (node->m_Bounds[2].Z - halfLength) * m_TerrScale;
	bounds[2].Z = 0;

	bounds[3].X = (node->m_Bounds[3].X - halfWidth) * m_TerrScale;
	bounds[3].Y = (node->m_Bounds[3].Z - halfLength) * m_TerrScale;
	bounds[3].Z = 0;

	//Add the height in
	if (m_UseHeight)
	{

	}

	FGeneratedMeshTriangle tri1;
	FGeneratedMeshTriangle tri2;
	
	//Create two triangles based on the bounds for this mesh
	tri1.Vertex0 = bounds[0];
	tri1.Vertex1 = bounds[3];
	tri1.Vertex2 = bounds[1];

	tri2.Vertex0 = bounds[0];
	tri2.Vertex1 = bounds[2];
	tri2.Vertex2 = bounds[3];

	m_Triangles.Add(tri1);
	m_Triangles.Add(tri2);
}

void AQuadTree::LoadQuadTree()
{
	LoadMap();
	Initialize();
	Create();
	GenerateMesh();
}

void AQuadTree::LoadMap(AHeightmap* heightmap)
{
	
}

void AQuadTree::LoadMap()
{
	for (int z = 0; z < m_QuadTreeSize + 1; z++)
	{
		for (int x = 0; x < m_QuadTreeSize + 1; x++)
		{
			m_Vertices.Add(FVector(x, 0, z));
		}
	}
}

int32 AQuadTree::SampleHeight(int32 index)
{
	return 0;
}

int32 AQuadTree::SampleHeight(int32 x, int32 z)
{
	return 0;
}

void AQuadTree::Initialize()
{
	m_TotalTreeID = 0;

	m_TotalLeaves = (m_QuadTreeSize / (m_LeafWidth - 1)) * (m_QuadTreeSize / (m_LeafWidth - 1));

	m_NodeCount = NumberOfNodes(m_TotalLeaves, m_LeafWidth - 1);
}

void AQuadTree::Create()
{
	FVector RootBounds[4];
	
	RootBounds[0].X = 0;
	RootBounds[0].Y = 0;
	RootBounds[0].Z = 0;

	RootBounds[1].X = m_QuadTreeSize;
	RootBounds[1].Y = 0;
	RootBounds[1].Z = 0;

	RootBounds[2].X = 0;
	RootBounds[2].Y = 0;
	RootBounds[2].Z = m_QuadTreeSize;

	RootBounds[3].X = m_QuadTreeSize;
	RootBounds[3].Y = 0;
	RootBounds[3].Z = m_QuadTreeSize;

	CreateNode(m_nodes, RootBounds, 0, 0);
}

int32 AQuadTree::NumberOfNodes(int32 leaves, int32 leafWidth)
{
	int ctr = 0, val = 0;

	while (val == 0)
	{
		ctr += leaves;
		leaves /= leafWidth;

		if (leaves == 0)
			break;

		if (leaves == 1)
			val = 1;
	}

	ctr++;

	return ctr;
}

void AQuadTree::CreateNode(FAQuadNode*& node, FVector bounds[4], int32 parentID, int32 nodeID)
{
	node = new FAQuadNode();

	node->m_Children[0] = 0;
	node->m_Children[1] = 0;
	node->m_Children[2] = 0;
	node->m_Children[3] = 0;

	float xDiff = bounds[0].X - bounds[1].X;
	float zDiff = bounds[0].Z - bounds[1].Z;

	//Find the width and height of the node
	int NodeWidth = (int)abs(xDiff);
	int NodeHeight = (int)abs(zDiff);

	EEQuadNodeType type;

	if (NodeWidth == m_LeafWidth - 1)
		type = LEAF;
	else
		type = NODE;

	node->m_Type = type;
	node->m_pNodeID = nodeID;
	node->m_ParentID = parentID;

	int bounds0X = (int)bounds[0].X;
	int bounds0Z = (int)bounds[0].Z;

	int bounds1X = (int)bounds[1].X;
	int bounds1Z = (int)bounds[1].Z;

	int bounds2X = (int)bounds[2].X;
	int bounds2Z = (int)bounds[2].Z;

	int bounds3X = (int)bounds[3].X;
	int bounds3Z = (int)bounds[3].Z;

	float height0 = 0;
	float height1 = 0;
	float height2 = 0;
	float height3 = 0;

	if (m_UseHeight)
	{
		if (bounds0X < m_QuadTreeSize && bounds0Z < m_QuadTreeSize) { height0 = m_Heightmap->SampleHeight((bounds0Z * m_QuadTreeSize) + bounds0X); }
		if (bounds1X < m_QuadTreeSize && bounds1Z < m_QuadTreeSize) { height1 = m_Heightmap->SampleHeight((bounds1Z * m_QuadTreeSize) + bounds1X); }
		if (bounds2X < m_QuadTreeSize && bounds2Z < m_QuadTreeSize) { height2 = m_Heightmap->SampleHeight((bounds2Z * m_QuadTreeSize) + bounds2X); }
		if (bounds3X < m_QuadTreeSize && bounds3Z < m_QuadTreeSize) { height3 = m_Heightmap->SampleHeight((bounds3Z * m_QuadTreeSize) + bounds3X); }
	}

	//Need to get the bounding coord for this node
	node->m_Bounds.Add(FVector(bounds0X, height0, bounds0Z));
	node->m_Bounds.Add(FVector(bounds1X, height1, bounds1Z));
	node->m_Bounds.Add(FVector(bounds2X, height2, bounds2Z));
	node->m_Bounds.Add(FVector(bounds3X, height3, bounds3Z));

	if (type == LEAF)
	{
		return;
	}
	else//Create a node
	{
		#pragma region "Child Node 1"
		//======================================================================================================================
		//Child Node 1
		m_TotalTreeID++;
		node->m_Branches[0] = m_TotalTreeID;
		FVector ChildBounds1[4];
		
		//Top-Left
		ChildBounds1[0].X = bounds[0].X;
		ChildBounds1[0].Y = 0;
		ChildBounds1[0].Z = bounds[0].Z;

		//Top-Right
		ChildBounds1[1].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds1[1].Y = 0;
		ChildBounds1[1].Z = bounds[1].Z;

		//Bottom-Left
		ChildBounds1[2].X = bounds[2].X;
		ChildBounds1[2].Y = 0;
		ChildBounds1[2].Z = bounds[0].Z + abs((bounds[0].Z - bounds[2].Z) / 2);

		//Bottom-Right
		ChildBounds1[3].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds1[3].Y = 0;
		ChildBounds1[3].Z = bounds[0].Z + abs((bounds[0].Z - bounds[2].Z) / 2);

		CreateNode(node->m_Children[0], ChildBounds1, nodeID, m_TotalTreeID);
		//======================================================================================================================
		#pragma endregion

		#pragma region "Child Node 2"
		//======================================================================================================================
		//Child Node 2
		m_TotalTreeID++;
		node->m_Branches[1] = m_TotalTreeID;
		FVector ChildBounds2[4];

		//Top-Left
		ChildBounds2[0].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds2[0].Y = 0;
		ChildBounds2[0].Z = bounds[1].Z;

		//Top-Right
		ChildBounds2[1].X = bounds[1].X;
		ChildBounds2[1].Y = 0;
		ChildBounds2[1].Z = bounds[1].Z;

		//Bottom-Left
		ChildBounds2[2].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds2[2].Y = 0;
		ChildBounds2[2].Z = bounds[1].Z + abs((bounds[1].Z - bounds[3].Z) / 2);

		//Bottom-Right
		ChildBounds2[3].X = bounds[1].X;
		ChildBounds2[3].Y = 0;
		ChildBounds2[3].Z = bounds[1].Z + abs((bounds[1].Z - bounds[3].Z) / 2);

		CreateNode(node->m_Children[1], ChildBounds2, nodeID, m_TotalTreeID);
		//======================================================================================================================
		#pragma endregion

		#pragma region "Child Node 3"
		//======================================================================================================================
		//Child Node 3
		m_TotalTreeID++;
		node->m_Branches[2] = m_TotalTreeID;
		FVector ChildBounds3[4];

		//Top-Left
		ChildBounds3[0].X = bounds[0].X;
		ChildBounds3[0].Y = 0;
		ChildBounds3[0].Z = bounds[0].Z + abs((bounds[0].Z - bounds[2].Z) / 2);

		//Top-Right
		ChildBounds3[1].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds3[1].Y = 0;
		ChildBounds3[1].Z = bounds[0].Z + abs((bounds[0].Z - bounds[2].Z) / 2);

		//Bottom-Left
		ChildBounds3[2].X = bounds[2].X;
		ChildBounds3[2].Y = 0;
		ChildBounds3[2].Z = bounds[2].Z;

		//Bottom-Right
		ChildBounds3[3].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds3[3].Y = 0;
		ChildBounds3[3].Z = bounds[3].Z;

		CreateNode(node->m_Children[2], ChildBounds3, nodeID, m_TotalTreeID);
		//======================================================================================================================
		#pragma endregion

		#pragma region "Child Node 4"
		//======================================================================================================================
		//Child Node 4
		m_TotalTreeID++;
		node->m_Branches[3] = m_TotalTreeID;
		FVector ChildBounds4[4];

		//Top-Left
		ChildBounds4[0].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds4[0].Y = 0;
		ChildBounds4[0].Z = bounds[1].Z + abs((bounds[1].Z - bounds[3].Z) / 2);

		//Top-Right
		ChildBounds4[1].X = bounds[3].X;
		ChildBounds4[1].Y = 0;
		ChildBounds4[1].Z = bounds[1].Z + abs((bounds[1].Z - bounds[3].Z) / 2);//-1

		//Bottom-Left
		ChildBounds4[2].X = bounds[0].X + abs((bounds[0].X - bounds[1].X) / 2);
		ChildBounds4[2].Y = 0;
		ChildBounds4[2].Z = bounds[3].Z;

		//Bottom-Right
		ChildBounds4[3].X = bounds[3].X;
		ChildBounds4[3].Y = 0;
		ChildBounds4[3].Z = bounds[3].Z;

		CreateNode(node->m_Children[3], ChildBounds4, nodeID, m_TotalTreeID);
		//======================================================================================================================
		#pragma endregion
	}
}

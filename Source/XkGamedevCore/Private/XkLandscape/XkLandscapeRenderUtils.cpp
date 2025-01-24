// Copyright ©ICEPRINCE. All Rights Reserved.


#include "XkLandscape/XkLandscapeRenderUtils.h"


FQuadtreeNode::FQuadtreeNode()
{
	Parent = -1;
	Children[0] = Children[1] = Children[2] = Children[3] = -1;
	Quadtree = nullptr;
}


FQuadtreeNode::~FQuadtreeNode()
{
	Parent = -1;
	Children[0] = Children[1] = Children[2] = Children[3] = -1;
	Quadtree = nullptr;
}


void FQuadtreeNode::Cull(TArray<int32>& OutNodes, const FConvexVolume* InCamera, const FVector& InCameraPos, const int8 InDepth, const int8 InDepthClip, const int16 InFrameTag)
{
	SCOPED_NAMED_EVENT(FQuadtreeNode_Cull, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FQuadtreeNode_Cull);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_FQuadtreeNode_Cull);

	// process and update node with extra info
	Quadtree->ProcessNodeFunc(*this, InCameraPos, NodeID);

	FVector RootOffset = Quadtree->RootOffset;
	FVector vCenter3D = NodeBox.GetCenter();
	vCenter3D += RootOffset;
	FVector vExtent3D = NodeBox.GetExtent();

	float fExtent = vExtent3D.X;
	float fNodeSize = fExtent * 2;
	bool bRootNode = fNodeSize > Quadtree->MaxNodeSize * Quadtree->UnrealUnitScale;
	// Scale node extent a little bit to avoid some xy displacement which might show a culled node
	vExtent3D += FVector(100.0);
	bool bIntersect = InCamera->IntersectBox(vCenter3D, vExtent3D);
	if (bIntersect)
	{
		float Dist0 = FVector::Dist2D(InCameraPos, vCenter3D + FVector(-fExtent, -fExtent, 0));
		float Dist1 = FVector::Dist2D(InCameraPos, vCenter3D + FVector(fExtent, -fExtent, 0));
		float Dist2 = FVector::Dist2D(InCameraPos, vCenter3D + FVector(-fExtent, fExtent, 0));
		float Dist3 = FVector::Dist2D(InCameraPos, vCenter3D + FVector(fExtent, fExtent, 0));

		float MinDist2D = FMath::Min(Dist0, Dist1);
		MinDist2D = FMath::Min(MinDist2D, Dist2);
		MinDist2D = FMath::Min(MinDist2D, Dist3);

		bool bRenderNode = false;
		// node depth clip
		if (InDepthClip == InDepth)
		{
			bRenderNode = true;
		}

		if (!bRootNode)
		{
			float fTestDist = Quadtree->CullLodDistance[InDepth];
			if (MinDist2D < fTestDist)
			{
				if (Children[0] == -1) // leaf node
				{
					bRenderNode = true;
				}
				else
				{
					float fTestDistChild = Quadtree->CullLodDistance[InDepth + 1];
					if (MinDist2D >= fTestDistChild)
					{
						bRenderNode = true;
					}
				}
			}
			else
			{
				float fTestDistParent = Quadtree->CullLodDistance[InDepth - 1] * 2.0;
				if (MinDist2D < fTestDistParent)
				{
					bRenderNode = true;
				}
			}
		}

		if (bRenderNode)
		{
			OutNodes.Add(NodeID);
			FrameTag = InFrameTag;
		}
		else
		{
			if (Children[TL] != -1 && Children[TR] != -1 && Children[BL] != -1 && Children[BR] != -1)
			{
				TArray<FQuadtreeNode>& TreeNodes = Quadtree->TreeNodes;
				TreeNodes[Children[TL]].Cull(OutNodes, InCamera, InCameraPos, InDepth + 1, InDepthClip, InFrameTag);
				TreeNodes[Children[TR]].Cull(OutNodes, InCamera, InCameraPos, InDepth + 1, InDepthClip, InFrameTag);
				TreeNodes[Children[BL]].Cull(OutNodes, InCamera, InCameraPos, InDepth + 1, InDepthClip, InFrameTag);
				TreeNodes[Children[BR]].Cull(OutNodes, InCamera, InCameraPos, InDepth + 1, InDepthClip, InFrameTag);
			}
		}
	}
}


void FQuadtreeNode::Split(const int32 InDepth)
{
	SCOPED_NAMED_EVENT(FQuadtreeNode_Split, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FQuadtreeNode_Split);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_FQuadtreeNode_Split);

	const FVector Center = NodeBox.GetCenter();
	const FVector Extent = NodeBox.GetExtent();
	const FVector BoxMin = NodeBox.Min;
	const FVector BoxMax = NodeBox.Max;
	const FVector2D XExtent = FVector2D(Extent.X, 0.f);
	const FVector2D YExtent = FVector2D(0.f, Extent.Y);

	/************************************************************************
	 *  ___________max
	 * |     |     |
	 * |     |     |
	 * |-----c------
	 * |     |     |
	 * min___|_____|
	 *
	 * We create new quads by adding xExtent and yExtent
	 ************************************************************************/

	const FVector2D CC2D = FVector2D(Center.X, Center.Y);
	const FVector2D TM2D = CC2D + YExtent;
	const FVector2D ML2D = CC2D - XExtent;
	const FVector2D MR2D = CC2D + XExtent;
	const FVector2D BM2D = CC2D - YExtent;
	const FVector2D BL2D = FVector2D(BoxMin.X, BoxMin.Y);
	const FVector2D TR2D = FVector2D(BoxMax.X, BoxMax.Y);

	if (Children[TL] != -1 && Children[TR] != -1 && Children[BL] != -1 && Children[BR] != -1)
	{
		TArray<FQuadtreeNode>& TreeNodes = Quadtree->TreeNodes;
		TreeNodes[Children[TL]].Split(InDepth + 1);
		TreeNodes[Children[TR]].Split(InDepth + 1);
		TreeNodes[Children[BL]].Split(InDepth + 1);
		TreeNodes[Children[BR]].Split(InDepth + 1);
	}
	else
	{
		Children[TL] = Quadtree->AllocateNode(FBox2D(ML2D, TM2D), 2 * NodePosX, 2 * NodePosY, InDepth + 1, NodeID);
		Children[TR] = Quadtree->AllocateNode(FBox2D(CC2D, TR2D), 2 * NodePosX + 1, 2 * NodePosY, InDepth + 1, NodeID);
		Children[BL] = Quadtree->AllocateNode(FBox2D(BL2D, CC2D), 2 * NodePosX, 2 * NodePosY + 1, InDepth + 1, NodeID);
		Children[BR] = Quadtree->AllocateNode(FBox2D(BM2D, MR2D), 2 * NodePosX + 1, 2 * NodePosY + 1, InDepth + 1, NodeID);
	}
}


const int32 FQuadtree::UnrealUnitScale = 100;


FQuadtree::FQuadtree()
{
	MaxDistance = 1000 * UnrealUnitScale;

	MaxNodeSize = 512;

	MaxNodeMove = 128;

	WorldSize = 2048;

	MinNodeSize = 16;

	MaxDepth = FMath::Log2((float)WorldSize / (float)MinNodeSize) + 1;

	Init = false;
}


FQuadtree::~FQuadtree()
{
	Init = false;
}


void FQuadtree::Initialize(const int32 InWorldSize, const int32 InMinNodeSize)
{
	SCOPED_NAMED_EVENT(FQuadtree_Initialize, FColor::Red);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FQuadtree_Initialize);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(STAT_FQuadtree_Initialize);

	if (!Init)
	{
		Init = true;
		TreeNodes.Empty();

		WorldSize = InWorldSize;
		MinNodeSize = InMinNodeSize;
		MaxDepth = FMath::Log2((float)WorldSize / (float)MinNodeSize) + 1;

		float HalfWorldSize = WorldSize * UnrealUnitScale / 2.0;
		FBox2D WorldBox = FBox2D(FVector2D(-HalfWorldSize, -HalfWorldSize),
			FVector2D(HalfWorldSize, HalfWorldSize));

		// init tree nodes
		TreeNodes.Empty();
		int32 iLayer = GetTreeDepth(MinNodeSize);
		int32 iMaxCount = GetFullNodeCount(iLayer);
		TreeNodes.Reserve(iMaxCount);
		FQuadtreeNode TmpData;
		for (int32 i = 0; i < iMaxCount; i++)
		{
			TreeNodes.Add(TmpData);
		}

		float iNodeSize = WorldBox.Max.X - WorldBox.Min.X;
		int32 iNumNodes = 1;
		int32 iCurLayerNum = 1;

		RootOffset = FVector::ZeroVector;

		CullLodDistance.Empty();
		CullLodSizes.Empty();
		VisibleNodes.Reserve(1000);

		int32 RootNodeIndex = AllocateNode(WorldBox, 0, 0, 0, -1);
		FQuadtreeNode* pRootNode = &TreeNodes[RootNodeIndex];
		iNodeSize = WorldBox.Max.X - WorldBox.Min.X;
		CullLodSizes.Add(iNodeSize);
		// CDLOD morph need more continuous node
		CullLodDistance.Add(iNodeSize * 4);
		while (iNodeSize > (MinNodeSize + 1) * UnrealUnitScale)
		{
			iNodeSize /= 2.0f;
			pRootNode->Split(0);
			CullLodSizes.Add(iNodeSize);
			CullLodDistance.Add(iNodeSize * 4);
		}
	}
}


int32 FQuadtree::AllocateNode(const FBox2D& Box, int32 InPosx, int32 InPosy, int32 InDepth, int32 Parent)
{
	int32 NodeIndex = GetNodeIndex(InPosx, InPosy, InDepth);
	FQuadtreeNode& NewNode = TreeNodes[NodeIndex];
	NewNode.NodeID = NodeIndex;
	// NodeBox extent z is one, center at zero
	NewNode.NodeBox = FBox(FVector(Box.Min.X, Box.Min.Y, -UE_FLOAT_HUGE_DISTANCE), FVector(Box.Max.X, Box.Max.Y, UE_FLOAT_HUGE_DISTANCE));
	NewNode.NodePosX = InPosx;
	NewNode.NodePosY = InPosy;
	NewNode.Parent = Parent;
	NewNode.NodeDepth = InDepth;
	NewNode.Quadtree = this;
	return NodeIndex;
}


void FQuadtree::Cull(const FConvexVolume* InCamera, const FVector& InCameraPos, const FVector& InPosition, const int16 InFrameTag)
{
	UpdateCameraPos(InCameraPos, InPosition);

	if (TreeNodes.Num())
	{
		VisibleNodes.Empty(1000);
		FBox RootNodeBox = TreeNodes[0].GetNodeBox();
		float DistZ = RootNodeBox.GetCenter().Z + RootNodeBox.Max.Z + InCameraPos.Z;
		int8 ClipDepth = MaxDepth - FMath::Floor(FMath::Log2((float)DistZ / (float)(MinNodeSize * UnrealUnitScale / 2.0)));
		TreeNodes[0].Cull(VisibleNodes, InCamera, InCameraPos, 0 /* RootNode depth*/, ClipDepth, InFrameTag);
	}
}


void FQuadtree::UpdateCameraPos(const FVector& InCameraPos, const FVector& InPosition)
{
	int32 MoveUnit = MaxNodeMove * FQuadtree::UnrealUnitScale;
	float fXPos = FMath::Floor(InCameraPos.X / MoveUnit);
	float fYPos = FMath::Floor(InCameraPos.Y / MoveUnit);
	RootOffset.X = fXPos * MoveUnit;
	RootOffset.Y = fYPos * MoveUnit;
	RootOffset.Z = InPosition.Z;
}


void FQuadtree::ProcessNodeFunc(FQuadtreeNode& OutNode, const FVector& InCameraPos, const int32 InNodeID) 
{ 
	if (ProcessNode) // check if TFunction callable
	{
		ProcessNode(OutNode, InCameraPos, InNodeID);
	}
}


int32 FQuadtree::GetTreeDepth(const int32 InMinNodeSize) const
{
	float fLog = FMath::Log2((float)InMinNodeSize);
	float fLogMax = FMath::Log2((float)WorldSize);
	int32 iRet = fLogMax - fLog;
	return iRet;
}


int32 FQuadtree::GetChildIndex(const int32 InCurIndex, const FQuadtreeNode::EChildName InChild) const
{
	int32 iCurDepth = -1;
	for (int32 i = 0; i <= MaxDepth; i++)
	{
		int32 iNodeCount = GetFullNodeCount(i);
		if (iNodeCount > InCurIndex)
		{
			iCurDepth = i;
			break;
		}
	}
	int32 iOffset = 0;
	if (iCurDepth)
		iOffset = GetFullNodeCount(iCurDepth - 1);
	int32 iRet = InCurIndex * 4 + InChild;

	return iRet;
}


int32 FQuadtree::GetNodeIndex(const int32 InPosx, const int32 InPosy, const int32 InDepth) const
{
	int32 iTreeOffset = 0;
	if (InDepth)
	{
		iTreeOffset = GetFullNodeCount(InDepth - 1);
	}

	int32 iLayerOffet = GetLayerOffset(InPosx, InPosy, InDepth);
	return iLayerOffet + iTreeOffset;
}


int32 FQuadtree::GetLayerOffset(const int32 InPosx, const int32 InPosy, const int32 InDepth) const
{
	int32 iWidht = FMath::Pow(2.0, InDepth);
	int32 iRet = InPosy * iWidht + InPosx;
	return iRet;
}


int32 FQuadtree::GetNodeIndexByPos(const FBox2D InNodeBox, const int32 InDepth, const FVector& InRootOffset) const
{
	int32 iRet = 0;
	FVector2D vSize = InNodeBox.GetSize();
	float fX = InNodeBox.Min.X + InRootOffset.X + MAX_WORLD_SIZE * 100 / 2;
	float fY = InNodeBox.Min.Y + InRootOffset.Y + MAX_WORLD_SIZE * 100 / 2;
	int32 iX = FMath::RoundToInt(fX / vSize.X);
	int32 iY = FMath::RoundToInt(fY / vSize.Y);
	int32 iMaxY = FMath::RoundToInt(MAX_WORLD_SIZE * 100 / vSize.Y) - 1;
	iY = iMaxY - iY;
	iRet = GetNodeIndex(iX, iY, InDepth);
	return iRet;
}


int32 FQuadtree::GetFullNodeCount(const int32 FQuadtree) const
{
	int32 iN = FMath::Pow(4.0, FQuadtree + 1);
	return (iN - 1) / 3;
}


FLinearColor Hue2RGB(int32 Index)
{
	float hue = Index * 1.71f;
	hue = FMath::Frac(hue); //only use fractional part of hue, making it loop
	float r = FMath::Abs(hue * 6 - 3) - 1; //red
	float g = 2 - FMath::Abs(hue * 6 - 2); //green
	float b = 2 - FMath::Abs(hue * 6 - 4); //blue
	FLinearColor rgb = FLinearColor(
		FMath::Clamp(r, 0.0, 1.0), FMath::Clamp(g, 0.0, 1.0), FMath::Clamp(b, 0.0, 1.0)); //combine components
	return rgb;
}


float SphericalHeight(const FVector& CameraLocation, const FVector& WorldLocation)
{
	float x = FVector::Dist2D(CameraLocation, WorldLocation);
	const float a = -0.00001f;
	const float b = 6400.0f;
	float y = FMath::Pow(FMath::Max(x - b, 0.0f), 2.0f) * a;
	return y;
}
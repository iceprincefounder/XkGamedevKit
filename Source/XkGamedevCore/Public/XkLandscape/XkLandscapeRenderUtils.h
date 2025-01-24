// Copyright ©ICEPRINCE. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#define	MAX_WORLD_SIZE 16384
#define	MAX_VISIBLE_NODE_COUNT 1024
#define	FARMESH_VERTEX_COUNT 8


class FQuadtree;
struct FConvexVolume;
class FXkQuadtreeSceneProxy;


class FQuadtreeNode
{
public:
	/** DO NOT USE. This constructor is for internal usage only for hot-reload purposes. */
	FQuadtreeNode();

	virtual ~FQuadtreeNode();

	virtual void Cull(TArray<int32>& OutNodes, const FConvexVolume* InCamera, const FVector& InCameraPos, const int8 InDepth, const int8 InDepthClip, const int16 InFrameTag);

	virtual void Split(const int32 InDepth);

	virtual FBox GetNodeBox() const { return NodeBox; };
	virtual FBox& GetNodeBoxRaw() { return NodeBox; };
	virtual int32 GetNodeID() const { return NodeID; };
	virtual int32 GetNodePosX() const { return NodePosX; };
	virtual int32 GetNodePosY() const { return NodePosY; };
	virtual int32 GetNodeDepth() const { return NodeDepth; };
public:
	enum EChildName
	{
		TL = 0, // TopLeft
		TR = 1, // TopRight
		BL = 2, // BottomLeft
		BR = 3  // BottomRight
	};

private:
	FBox NodeBox;
	int32 NodeID;
	int32 NodePosX;
	int32 NodePosY;
	int8 NodeDepth;
	int16 FrameTag;

	int32 Parent;
	int32 Children[4];

	FQuadtree* Quadtree;

	friend FQuadtree;
};


class FQuadtree
{
	friend class FQuadtreeNode;
public:
	FQuadtree();

	virtual ~FQuadtree();

	virtual void Initialize(const int32 InMapSize, const int32 InMinNodeSize);

	virtual int32 AllocateNode(const FBox2D& Box, int32 InPosx, int32 InPosy, int32 InDepth, int32 Parent);

	/* Visible quadtree node cull (select node) */
	virtual void Cull(const FConvexVolume* InCamera, const FVector& InCameraPos, const FVector& InPosition, const int16 InFrameTag);

	virtual void UpdateCameraPos(const FVector& InCameraPos, const FVector& Position);

	virtual void InitProcessFunc(const TFunction<void(FQuadtreeNode&, const FVector&, const int32)>& Input) { ProcessNode = Input; };

	virtual void ProcessNodeFunc(FQuadtreeNode& OutNode, const FVector& InCameraPos, const int32 InNodeID);

	virtual int32 GetTreeDepth(const int32 InMinNodeSize) const;

	virtual int32 GetChildIndex(const int32 InCurIndex, const FQuadtreeNode::EChildName InChild) const;

	virtual int32 GetNodeIndex(const int32 InPosx, const int32 InPosy, const int32 InDepth) const;

	virtual int32 GetLayerOffset(const int32 InPosx, const int32 InPosy, const int32 InDepth) const;

	virtual int32 GetNodeIndexByPos(const FBox2D InNodeBox, const int32 InDepth, const FVector& InRootOffset) const;

	virtual int32 GetFullNodeCount(const int32 InLayerCount) const;

	virtual const TArray<FQuadtreeNode>& GetTreeNodes() const { return TreeNodes; }

	virtual const TArray<int32>& GetVisibleNodes() const { return VisibleNodes; }

	virtual const FVector GetRootOffset() const { return RootOffset; };

	virtual TArray<float> GetCullLodDistance() const { return CullLodDistance; };

	virtual TArray<float> GetCullLodSizes() const { return CullLodSizes; }

	virtual int32 GetMaxNodeSize() const { return MaxNodeSize; };

	virtual int32 GetMinNodeSize() const { return MinNodeSize; };

	virtual int32 GetMaxDepth() const { return MaxDepth; };

	static const int32 UnrealUnitScale;
private:
	TArray<FQuadtreeNode> TreeNodes;
	TArray<int32> VisibleNodes;
	FVector RootOffset;
	TArray<float> CullLodDistance;
	TArray<float> CullLodSizes;

	bool Init;
	float MaxDistance;
	int32 MaxNodeSize;
	int32 MinNodeSize;
	int32 MaxNodeMove;
	int32 MaxDepth;
	int32 WorldSize;
	
private:
	// std::function to post process node's center and extent
	TFunction<void(FQuadtreeNode&, const FVector& /*Camera Pos*/, const int32)> ProcessNode;
};

// https://www.ronja-tutorials.com/post/041-hsv-colorspace/
extern FLinearColor Hue2RGB(int32 Index);

extern float SphericalHeight(const FVector& CameraLocation, const FVector& WorldLocation);
#include <d2d1.h>
#include <dwrite.h>
#include <stdio.h>

#include <XLib.Types.h>
#include <XLib.Memory.h>
#include <XLib.Program.h>
#include <XLib.Vectors.h>
#include <XLib.Vectors.Arithmetics.h>
#include <XLib.Vectors.Math.h>
#include <XLib.System.Window.h>
#include <XLib.Containers.Vector.h>
#include <XLib.Algorithm.QuickSort.h>

using namespace XLib;
using namespace D2D1;

inline D2D1_POINT_2F d2d(float32x2 value) { return Point2F(value.x, value.y); }
inline D2D1_RECT_F d2d(rectf32 rect) { return RectF(rect.left, rect.top, rect.right, rect.bottom); }
inline D2D1_RECT_F d2d(rectf32 rect, float32 offset) { return RectF(rect.left + offset, rect.top + offset, rect.right - offset, rect.bottom - offset); }
inline D2D1_ELLIPSE d2dEllipse(const rectf32& rect)
{
	return Ellipse(Point2F((rect.left + rect.right) / 2.0f, (rect.top + rect.bottom) / 2.0f),
		(rect.right - rect.left) / 2.0f, (rect.bottom - rect.top) / 2.0f);
}

inline bool CheckPointInsideRectangle(const rectf32& rect, float32x2 point)
{
	return rect.left <= point.x && rect.right >= point.x &&
		rect.top <= point.y && rect.bottom >= point.y;
}

inline rectf32 AABB(const rectf32& a, const rectf32 b)
{
	return rectf32(
		min(a.left, b.left), min(a.top, b.top),
		max(a.right, b.right), max(a.bottom, b.bottom));
}

inline float32 S(const rectf32& a)
{
	return abs((a.right - a.left) * (a.bottom - a.top));
}

struct BVHNode
{
	rectf32 aabb;
	uint32 leftChildId, rightChildId;
	bool isLeaf;

	inline operator rectf32() const { return aabb; }
};

class MainWindow : public WindowBase
{
private:
	ID2D1Factory *d2dFactory;
	ID2D1HwndRenderTarget *d2dRT;
	ID2D1SolidColorBrush *d2dSolidBrush;

	IDWriteFactory *dwFactory;
	IDWriteTextFormat *dwTextFormat;

	BVHNode *selectedNode = nullptr;
	sint16x2 lastPointerPosition;
	bool nodeResize = false;

	Vector<BVHNode> bvh;
	uint32 rootId = 0;

	virtual void onCreate(CreationArgs& args) override
	{
		DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, __uuidof(IDWriteFactory), (IUnknown**)&dwFactory);
		dwFactory->CreateTextFormat(L"SegoeUI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL, 18.0f, L"en-us", &dwTextFormat);

		D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory);
		d2dFactory->CreateHwndRenderTarget(RenderTargetProperties(),
			HwndRenderTargetProperties(HWND(getHandle()), SizeU(args.width, args.height)), &d2dRT);
		d2dRT->CreateSolidColorBrush(ColorF(0), &d2dSolidBrush);

		/*{
			BVHNode node;
			node.aabb = { 10.0f, 10.0f, 100.0f, 200.0f };
			node.isLeaf = false;
			bvh.pushBack(node);
		}

		{
			BVHNode node;
			node.aabb = { 100.0f, 309.0f, 120.0f, 340.0f };
			node.isLeaf = false;
			bvh.pushBack(node);
		}

		{
			BVHNode node;
			node.aabb = { 155.0f, 339.0f, 520.0f, 400.0f };
			node.isLeaf = true;
			bvh.pushBack(node);
		}*/
	}
	virtual void onMouseButton(MouseState& mouseState, MouseButton button, bool state) override
	{
		lastPointerPosition = { mouseState.x, mouseState.y };

		if (state)
		{
			float32x2 pointer = float32x2(mouseState.x, mouseState.y);
			if (button == MouseButton::Right)
			{
				for each (BVHNode& node in bvh)
				{
					if (node.isLeaf && CheckPointInsideRectangle(node.aabb, pointer))
					{
						nodeResize = false;
						selectedNode = &node;
						break;
					}
				}
			}
			else if (button == MouseButton::Left)
			{
				BVHNode node;
				node.aabb.left = pointer.x - 5.0f;
				node.aabb.top = pointer.y - 5.0f;
				node.aabb.right = pointer.x;
				node.aabb.bottom = pointer.y;
				node.isLeaf = true;
				nodeResize = true;
				selectedNode = &bvh.pushBack(node);

				if (bvh.getSize() != 1)
				{
					BVHNode root;
					root.aabb = AABB(bvh[rootId].aabb, node.aabb);
					root.isLeaf = false;
					root.leftChildId = rootId;
					root.rightChildId = bvh.getSize() - 1;
					bvh.pushBack(root);
					rootId = bvh.getSize() - 1;
				}
			}
		}
		else
		{
			selectedNode = nullptr;
		}

		/*if (state)
		{
			float32x2 pointer = float32x2(mouseState.x, mouseState.y);
			if (button == MouseButton::Right)
				points.pushBack(pointer);
			else if (button == MouseButton::Left)
			{
				if (VectorMath::Length(selectionRectVertex1 - pointer) < 10.0f)
				{
					selectedPointId = 0;
				}
				else if (VectorMath::Length(selectionRectVertex2 - pointer) < 10.0f)
				{
					selectedPointId = 1;
				}
				else
				{
					for (uint32 i = 0; i < points.getSize(); i++)
					{
						if (VectorMath::Length(points[i] - pointer) < 10.0f)
						{
							selectedPointId = i + 2;
							break;
						}
					}
				}
			}
		}
		else
		{
			if (button == MouseButton::Left)
				selectedPointId = uint32(-1);
		}*/
	}
	virtual void onMouseMove(MouseState& state) override
	{
		if (!selectedNode)
			return;

		float32x2 translation(float32(state.x) - lastPointerPosition.x, float32(state.y) - lastPointerPosition.y);

		if (nodeResize)
		{
			selectedNode->aabb.right = state.x;
			selectedNode->aabb.bottom = state.y;
		}
		else
		{
			selectedNode->aabb.left += translation.x;
			selectedNode->aabb.top += translation.y;
			selectedNode->aabb.right += translation.x;
			selectedNode->aabb.bottom += translation.y;
		}

		lastPointerPosition = { state.x, state.y };
	}
	virtual void onKeyboard(VirtualKey key, bool state) override
	{
		switch (key)
		{
		case VirtualKey('A'):
			updateAABBs();
			break;

		case VirtualKey::Escape:
			destroy();
			break;
		}
	}

	void updateAABBs()
	{
		for (uint32 i = 0; i < 100; i++)
		{
			for each (BVHNode& node in bvh)
			{
				if (node.isLeaf)
					continue;

				node.aabb = AABB(bvh[node.leftChildId].aabb, bvh[node.rightChildId].aabb);
			}
		}
	}
	
	void optimizeNode(BVHNode& node)
	{
		BVHNode &L = bvh[node.leftChildId];
		BVHNode &R = bvh[node.rightChildId];

		if (L.isLeaf)
		{
			if (!R.isLeaf)
				optimizeNode(R);
			return;
		}

		if (R.isLeaf)
		{
			if (!L.isLeaf)
				optimizeNode(L);
			return;
		}

		BVHNode &A = bvh[L.leftChildId];
		BVHNode &B = bvh[L.rightChildId];
		BVHNode &C = bvh[R.leftChildId];
		BVHNode &D = bvh[R.rightChildId];

		//     root
		//    /    \
		//   L      R
		//  / \    / \
		// A   B  C   D
		//
		// AB CD   AC BD   AD CB
		//
		// A <-> R    (R B) + A
		// B <-> R    (A R) + B
		// D <-> L    D + (C L)
		// C <-> L    C + (L D)
		//
		// A <-> C   (C B) + (A D)
		// A <-> D   (D B) + (C A)

		float32 sAR = S(AABB(R, B)) + S(A);
		float32 sBR = S(AABB(A, R)) + S(B);
		float32 sDL = S(D) + S(AABB(C, L));
		float32 sCL = S(C) + S(AABB(L, D));

		float32 sAC = S(AABB(C, B)) + S(AABB(A, D));
		float32 sAD = S(AABB(D, B)) + S(AABB(C, A));
	}

public:
	void update()
	{
		d2dRT->BeginDraw();
		d2dRT->Clear(ColorF(ColorF::White));

		d2dRT->SetTransform(Matrix3x2F::Translation(SizeF(0.5f, 0.5f)));

		for each (BVHNode& node in bvh)
		{
			if (node.isLeaf)
			{
				d2dSolidBrush->SetColor(ColorF(ColorF::Blue, 0.6f));
				d2dRT->FillEllipse(d2dEllipse(node.aabb), d2dSolidBrush);
			}
			else
			{
				d2dSolidBrush->SetColor(ColorF(ColorF::Red, 0.2f));
				d2dRT->FillRectangle(d2d(node.aabb), d2dSolidBrush);

				d2dSolidBrush->SetColor(ColorF(ColorF::Red));
				d2dRT->DrawRectangle(d2d(node.aabb), d2dSolidBrush, 1.0f);
			}
		}

		d2dRT->EndDraw();
	}
};

void Program::Run()
{
	MainWindow window;
	window.create(1280, 900, L"Lab 3. kd tree");
	while (window.isOpened())
	{
		WindowBase::DispatchPending();
		window.update();
	}
}
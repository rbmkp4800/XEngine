#pragma once

namespace XEngine
{
	class XERGeometry;
	class XERDevice;

	class XERGeometryGenerator abstract final
	{
	public:
		static void Triangle(XERDevice* xerDevice, XERGeometry* gemometry);
		static void Plane(XERDevice* device, XERGeometry* geometry);
		static void Cube(XERDevice* device, XERGeometry* geometry);
		static void Tetrahedron(XERDevice* device, XERGeometry* geometry);
		static void Sphere(uint32 detalizationLevel, XERDevice* device, XERGeometry* geometry);
		//static void Monster(XERDevice* device, XERGeometry* geometry);
		static void MonsterSkinned(XERDevice* device, XERGeometry* geometry);
	};
}
#include <string>
#include <vector>

#include <maya/MPlugArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MPointArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MAnimControl.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshVertex.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MDagPath.h>
#include <maya/MEvaluationNode.h>
#include <maya/MSelectionList.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MHardwareRenderer.h>
#include <maya/MGLFunctionTable.h>

#include "controlShape.h"

#define MSTR(v) MString(to_string(v).c_str())

MTypeId ControlShape::typeId(1274448);
MString ControlShape::drawDbClassification = "drawdb/geometry/controlShape";
MString ControlShape::drawRegistrantId = "controlShapePlugin";

MObject ControlShape::attr_mesh;
MObject ControlShape::attr_meshFaces;
MObject ControlShape::attr_meshMatrix;
MObject ControlShape::attr_facesOffset;
MObject ControlShape::attr_localTranslate;
MObject ControlShape::attr_localRotateX;
MObject ControlShape::attr_localRotateY;
MObject ControlShape::attr_localRotateZ;
MObject ControlShape::attr_localRotate;
MObject ControlShape::attr_localScaleX;
MObject ControlShape::attr_localScaleY;
MObject ControlShape::attr_localScaleZ;
MObject ControlShape::attr_localScale;
MObject ControlShape::attr_color;
MObject ControlShape::attr_opacity;
MObject ControlShape::attr_selectionOpacity;
MObject ControlShape::attr_meshInWorldSpace;

MGLFunctionTable *gGLFT = MHardwareRenderer::theRenderer()->glFunctionTable();

bool isPathSelected(const MDagPath& path)
{
	MSelectionList sel;
	MGlobal::getActiveSelectionList(sel);
	if (sel.hasItem(path))
		return true;
	
	const MDagPath transform = MDagPath::getAPathTo(path.transform());
	if (sel.hasItem(transform))
		return true;

	return false;
}

void ControlShape::postConstructor()
{
	thisNodeFn.setObject(thisMObject());
	thisNodeFn.findPlug("hideOnPlayback", false).setBool(true);
}
MSelectionMask ControlShape::getShapeSelectionMask() const
{
	MSelectionMask mask;
	mask.addMask(MSelectionMask::kSelectMeshes);
	mask.addMask(MSelectionMask::kSelectJoints);
	return mask;
}

void ControlShape::getMeshInfo(MPointArray &trianglePoints, MBoundingBox &bbox) const
{
	ControlShape* ptr = const_cast<ControlShape*>(this); // Cast away the constant
	MDataBlock dataBlock = ptr->forceCache();

	MObject mesh = dataBlock.inputValue(attr_mesh).asMesh();

	if (!mesh.isNull())
	{
		const bool meshInWorldSpace = dataBlock.inputValue(attr_meshInWorldSpace).asBool();
		
		const MMatrix meshMatrix = dataBlock.inputValue(attr_meshMatrix).asMatrix();
		const MPoint localTranslate = dataBlock.inputValue(attr_localTranslate).asFloatVector();
		const MVector localRotate = dataBlock.inputValue(attr_localRotate).asVector();
		const MVector localScale = dataBlock.inputValue(attr_localScale).asFloatVector();
		
		MTransformationMatrix trm(meshMatrix);
		trm.addTranslation(localTranslate, MSpace::kObject);
		const double rotation[3] = { localRotate.x, localRotate.y, localRotate.z };
		trm.addRotation(rotation, MTransformationMatrix::RotationOrder::kXYZ, MSpace::kObject);

		const double scale[3] = {localScale.x, localScale.y, localScale.z };
		trm.addScale(scale, MSpace::kObject);
		const MMatrix finalMeshMatrix = trm.asMatrix();
	
		MDagPath path = MDagPath::getAPathTo(thisMObject());
		const MMatrix mat = meshInWorldSpace ? finalMeshMatrix * path.inclusiveMatrixInverse() : finalMeshMatrix;

		const float facesOffset = dataBlock.inputValue(attr_facesOffset).asFloat();
		MFnIntArrayData meshFacesDataFn(dataBlock.inputValue(attr_meshFaces).data());

		MFnMesh meshFn(mesh);		

		const float* rawPoints = meshFn.getRawPoints(nullptr);

		if (meshFacesDataFn.length() > 0) // use meshFaces only
		{
			for (int i = 0; i < meshFacesDataFn.length(); i++)
			{
				const int faceId = meshFacesDataFn[i];

				MIntArray vertexList;
				meshFn.getPolygonVertices(faceId, vertexList);

				MVectorArray normals(vertexList.length());
				for (int k = 0; k < vertexList.length(); k++)
					meshFn.getVertexNormal(vertexList[k], false, normals[k]);

				vector<int> vertexOrder;
				if (vertexList.length() == 3)
					vertexOrder = { 0, 1, 2 };

				else if (vertexList.length() == 4)
					vertexOrder = { 0, 1, 2, 2, 3, 0 };

				for (const int& k : vertexOrder)
				{
					const int& idx = vertexList[k];

					MPoint p(rawPoints[idx * 3 + 0], rawPoints[idx * 3 + 1], rawPoints[idx * 3 + 2]);
					p += normals[k] * facesOffset;
					p *= mat;

					trianglePoints.append(p);
					bbox.expand(p);
				}
			}
		}
		else // use whole mesh
		{
			MFloatVectorArray normals;
			meshFn.getVertexNormals(false, normals);

			MIntArray triangleCounts, triangleVertices;
			meshFn.getTriangles(triangleCounts, triangleVertices);

			trianglePoints.setLength(triangleVertices.length());

			for (int i = 0; i< triangleVertices.length();i++)
			{
				const int idx = triangleVertices[i];
				MPoint p(rawPoints[idx * 3 + 0], rawPoints[idx * 3 + 1], rawPoints[idx * 3 + 2]);
				p += normals[idx] * facesOffset;
				p *= mat;

				trianglePoints[i] = p;
				bbox.expand(p);
			}			
		}
	}
}

MColor ControlShape::getColor() const
{
	float a = MPlug(thisMObject(), attr_opacity).asFloat();

	MPlug plug(thisMObject(), attr_color);
	float r, g, b;
	MFnNumericData(plug.asMObject()).getData(r, g, b);
	return MColor(r, g, b, a);
}

MColor ControlShape::getSelectionColor() const
{
	float a = MPlug(thisMObject(), attr_selectionOpacity).asFloat();
	MColor color = M3dView::leadColor();
	color.a = a;
	return color;
}

MBoundingBox ControlShape::boundingBox() const
{
	MPointArray points;
	MBoundingBox bbox;
	getMeshInfo(points, bbox);
	return bbox;
}

MStatus ControlShape::setDependentsDirty(const MPlug& plug, MPlugArray& plugArray)
{
	const MPlug plugParent = plug.parent();

	if (plug == attr_mesh ||
		plug == attr_meshFaces ||
		plug == attr_meshMatrix ||
		plug == attr_localTranslate || plugParent == attr_localTranslate ||
		plug == attr_localRotate || plugParent == attr_localRotate ||
		plug == attr_localScale || plugParent == attr_localScale ||
		plug == attr_color ||
		plug == attr_opacity ||
		plug == attr_selectionOpacity ||
		plug == attr_meshInWorldSpace ||
		plug == attr_facesOffset)
	{
		MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());
	}

	return MS::kSuccess;
}

MStatus ControlShape::preEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode)
{	
	if (context.isNormal())
	{
		MStatus status;
		if ((evaluationNode.dirtyPlugExists(attr_mesh, &status) && status) ||
			(evaluationNode.dirtyPlugExists(attr_meshFaces, &status) && status) ||
			(evaluationNode.dirtyPlugExists(attr_meshMatrix, &status) && status) ||
			(evaluationNode.dirtyPlugExists(attr_localTranslate, &status) && status) ||
			(evaluationNode.dirtyPlugExists(attr_localRotate, &status) && status) ||
			(evaluationNode.dirtyPlugExists(attr_localScale, &status) && status) ||
			(evaluationNode.dirtyPlugExists(attr_color, &status) && status) ||
			(evaluationNode.dirtyPlugExists(attr_opacity, &status) && status) ||
			(evaluationNode.dirtyPlugExists(attr_selectionOpacity, &status) && status) ||
			(evaluationNode.dirtyPlugExists(attr_meshInWorldSpace, &status) && status) ||
			(evaluationNode.dirtyPlugExists(attr_facesOffset, &status) && status))
		{
			MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());
		}
	}
	return MS::kSuccess;
}

MStatus ControlShape::postEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode, PostEvaluationType evalType)
{
	return MStatus::kSuccess;
}


MStatus ControlShape::initialize()
{
	MFnNumericAttribute nAttr;
	MFnTypedAttribute tAttr;
	MFnUnitAttribute uAttr;
	MFnMatrixAttribute mAttr;

	attr_mesh = tAttr.create("mesh", "mesh", MFnData::kMesh);
	tAttr.setHidden(true);
	addAttribute(attr_mesh);

	attr_meshFaces = tAttr.create("meshFaces", "meshFaces", MFnNumericData::kIntArray);
	addAttribute(attr_meshFaces);

	attr_meshMatrix = mAttr.create("meshMatrix", "meshMatrix");
	mAttr.setHidden(true);
	addAttribute(attr_meshMatrix);

	attr_facesOffset = nAttr.create("facesOffset", "facesOffset", MFnNumericData::kFloat, 0.1);
	nAttr.setKeyable(true);
	addAttribute(attr_facesOffset);

	attr_localTranslate = nAttr.createPoint("localTranslate", "localTranslate");
	nAttr.setChannelBox(true);
	addAttribute(attr_localTranslate);

	attr_localRotateX = uAttr.create("localRotateX", "localRotateX", MFnUnitAttribute::kAngle, 0.0);
	attr_localRotateY = uAttr.create("localRotateY", "localRotateY", MFnUnitAttribute::kAngle, 0.0);
	attr_localRotateZ = uAttr.create("localRotateZ", "localRotateZ", MFnUnitAttribute::kAngle, 0.0);
	attr_localRotate = nAttr.create("localRotate", "localRotate", attr_localRotateX, attr_localRotateY, attr_localRotateZ);
	nAttr.setChannelBox(true);
	addAttribute(attr_localRotate);

	attr_localScaleX = nAttr.create("localScaleX", "localScaleX", MFnNumericData::kFloat, 1);
	attr_localScaleY = nAttr.create("localScaleY", "localScaleY", MFnNumericData::kFloat, 1);
	attr_localScaleZ = nAttr.create("localScaleZ", "localScaleZ", MFnNumericData::kFloat, 1);
	attr_localScale = nAttr.create("localScale", "localScale", attr_localScaleX, attr_localScaleY, attr_localScaleZ);
	nAttr.setChannelBox(true);
	addAttribute(attr_localScale);

	attr_color = nAttr.createColor("color", "color");
	nAttr.setDefault(1.0f, 1.0f, 0.00f);
	addAttribute(attr_color);

	attr_opacity = nAttr.create("opacity", "opacity", MFnNumericData::kFloat, 0.15);
	nAttr.setMin(0);
	nAttr.setMax(1);
	nAttr.setKeyable(true);
	addAttribute(attr_opacity);

	attr_selectionOpacity = nAttr.create("selectionOpacity", "selectionOpacity", MFnNumericData::kFloat, 0.2);
	nAttr.setMin(0);
	nAttr.setMax(1);
	nAttr.setKeyable(true);
	addAttribute(attr_selectionOpacity);

	attr_meshInWorldSpace = nAttr.create("meshInWorldSpace", "meshInWorldSpace", MFnNumericData::kBoolean, false);
	addAttribute(attr_meshInWorldSpace);

	return MS::kSuccess;
}

// --------------------------------------------------------------------------------------------

bool ControlShapeUI::select(MSelectInfo& selectInfo, MSelectionList& selectionList, MPointArray& worldSpaceSelectPts) const
{
	MGlobal::displayInfo("SELECT");
	/*
	const bool wireframe = (selectInfo.displayStyle() == M3dView::kWireFrame);
	MGlobal::displayInfo("w:"+MSTR(wireframe));
	if (wireframe)
		return false;

	const ControlShape* controlShape = (ControlShape*)surfaceShape();

	MObject mesh = controlShape->getMesh();
	if (mesh.isNull())
		return false;

	MIntArray triangleCounts, triangleVertices;
	MFnMesh meshFn(mesh);
	meshFn.getTriangles(triangleCounts, triangleVertices);
	MPointArray points;
	meshFn.getPoints(points);

	M3dView view = selectInfo.view();

	for (int i = 0; i < triangleVertices.length(); i+=3)
	{
		view.beginSelect();

		gGLFT->glBegin(GL_TRIANGLES);
			gGLFT->glVertex3f(points[triangleVertices[i+0]].x, points[triangleVertices[i+0]].y, points[triangleVertices[i+0]].z);
			gGLFT->glVertex3f(points[triangleVertices[i+1]].x, points[triangleVertices[i+1]].y, points[triangleVertices[i+1]].z);
			gGLFT->glVertex3f(points[triangleVertices[i+2]].x, points[triangleVertices[i+2]].y, points[triangleVertices[i+2]].z);
		gGLFT->glEnd();

		if (view.endSelect() > 0)
		{
			MSelectionList item;
			item.add(selectInfo.selectPath());

			MDagPath path;
			if (item.getDagPath(0, path) == MS::kSuccess)
			{
				const MMatrix mat = path.inclusiveMatrix();
				const MPoint point(mat[3][0], mat[3][1], mat[3][2]);

				MSelectionMask mask(MSelectionMask::kSelectJoints);
				selectInfo.addSelection(item, point, selectionList, worldSpaceSelectPts, mask, false);
				return true;
			}
		}
	}*/
	
	return false;	
} 

// --------------------------------------------------------------------------------

MMatrix ControlShapeDrawOverride::transform(const MDagPath& objPath, const MDagPath& cameraPath) const
{
	return objPath.inclusiveMatrix();
}

MUserData* ControlShapeDrawOverride::prepareForDraw(const MDagPath& objPath, const MDagPath& cameraPath, const MHWRender::MFrameContext& frameContext, MUserData* oldData)
{
	MStatus stat;

	MObject obj = objPath.node(&stat);
	if (stat != MS::kSuccess)
		return NULL;

	auto* data = dynamic_cast<ControlShapeDrawData*>(oldData);

	if (!data)
		data = new ControlShapeDrawData();

	data->selected = isPathSelected(objPath);

	MFnDependencyNode nodeFn(obj);
	ControlShape *controlShape = dynamic_cast<ControlShape*>(nodeFn.userNode());

	data->color = data->selected ? controlShape->getSelectionColor() : controlShape->getColor();
	
	data->trianglePoints.clear();
	controlShape->getMeshInfo(data->trianglePoints, data->boundingBox);
	return data;
}

void ControlShapeDrawOverride::addUIDrawables(const MDagPath& objPath, MHWRender::MUIDrawManager& drawManager, const MHWRender::MFrameContext& frameContext, const MUserData* userData)
{	
	auto* data = dynamic_cast<const ControlShapeDrawData*>(userData);
	if (data)
	{
		drawManager.beginDrawable();
		drawManager.setColor(data->color);
		drawManager.mesh(MUIDrawManager::kTriangles, data->trianglePoints);
		drawManager.endDrawable();
	}
}
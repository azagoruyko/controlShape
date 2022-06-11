#pragma	once

#include <maya/MPxNode.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MPointArray.h>

using namespace std;

class ControlShape : public MPxSurfaceShape
{
public:
	static MTypeId typeId;
	static MString drawDbClassification;
	static MString drawRegistrantId;

	static MObject attr_mesh;
	static MObject attr_meshFaces;
	static MObject attr_meshMatrix;
	static MObject attr_facesOffset;
	static MObject attr_localTranslate;
	static MObject attr_localRotateX;
	static MObject attr_localRotateY;
	static MObject attr_localRotateZ;
	static MObject attr_localRotate;
	static MObject attr_localScaleX;
	static MObject attr_localScaleY;
	static MObject attr_localScaleZ;
	static MObject attr_localScale;
	static MObject attr_color;
	static MObject attr_opacity;
	static MObject attr_selectionOpacity;
	static MObject attr_meshInWorldSpace;

	static void* creator() { return new ControlShape(); }

	void postConstructor();

	MSelectionMask getShapeSelectionMask() const;

	bool isBounded() const { return true; }
	MBoundingBox boundingBox() const;

	static MStatus initialize();

	MStatus setDependentsDirty(const MPlug& plug, MPlugArray& plugArray);
	MStatus preEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode);
	MStatus postEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode, PostEvaluationType evalType);

	void getMeshInfo(MPointArray&, MBoundingBox&) const;
	MColor getColor() const;
	MColor getSelectionColor() const;

private:
	MFnDependencyNode thisNodeFn;
};

// -------------------------------------------------------------------------------

class ControlShapeUI : public MPxSurfaceShapeUI
{
public:
	static void* creator() { return new ControlShapeUI(); }
	virtual bool select(MSelectInfo& selectInfo, MSelectionList& selectionList, MPointArray& worldSpaceSelectPts) const;
};

// --------------------------------------------------------------------------------

class ControlShapeDrawData : public MUserData
{
public:
	ControlShapeDrawData() : MUserData(true) {} // deleteAfterUse = true

	MPointArray trianglePoints;
	MColor color;
	bool selected;
	MBoundingBox boundingBox;
};

class ControlShapeDrawOverride : public MHWRender::MPxDrawOverride
{
public:
	static MHWRender::MPxDrawOverride* creator(const MObject& obj) { return new ControlShapeDrawOverride(obj); }

	MHWRender::DrawAPI supportedDrawAPIs() const { return MHWRender::kOpenGL | MHWRender::kDirectX11 | MHWRender::kOpenGLCoreProfile; }

	MUserData* prepareForDraw(const MDagPath& objPath, const MDagPath& cameraPath, const MHWRender::MFrameContext& frameContext, MUserData* oldData);

	bool hasUIDrawables() const { return true; }
	void addUIDrawables(const MDagPath& objPath, MHWRender::MUIDrawManager& drawManager, const MHWRender::MFrameContext& frameContext, const MUserData* data);
	MMatrix transform(const MDagPath& objPath, const MDagPath& cameraPath) const;

	
	/*
	bool isBounded(const MDagPath &objPath, const MDagPath &cameraPath) const { return true; }
	MBoundingBox boundingBox(const MDagPath &objPath, const MDagPath &cameraPath) const 
	{
		MFnDependencyNode nodeFn(objPath.node());
		const ControlShape *controlShape = (ControlShape *)nodeFn.userNode();
		return controlShape->boundingBox();
	}
	*/

private:
	ControlShapeDrawOverride(const MObject& obj) : MHWRender::MPxDrawOverride(obj, NULL, false) {} // alwaysDirty is false
};
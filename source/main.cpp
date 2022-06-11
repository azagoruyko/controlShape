#include <maya/MFnPlugin.h>
#include <maya/MDrawRegistry.h>

#include "controlShape.h"

MStatus initializePlugin(MObject plugin)
{
	MStatus stat;
	MFnPlugin pluginFn(plugin);

	stat = pluginFn.registerShape("controlShape", ControlShape::typeId, ControlShape::creator, ControlShape::initialize, ControlShapeUI::creator, &ControlShape::drawDbClassification);
	CHECK_MSTATUS_AND_RETURN_IT(stat);

	stat = MDrawRegistry::registerDrawOverrideCreator(ControlShape::drawDbClassification, ControlShape::drawRegistrantId, ControlShapeDrawOverride::creator);
	CHECK_MSTATUS_AND_RETURN_IT(stat);

	return MS::kSuccess;
}

MStatus uninitializePlugin(MObject plugin)
{
	MStatus stat;
	MFnPlugin pluginFn(plugin);

	stat = MDrawRegistry::deregisterDrawOverrideCreator(ControlShape::drawDbClassification, ControlShape::drawRegistrantId);
	CHECK_MSTATUS_AND_RETURN_IT(stat);

	stat = pluginFn.deregisterNode(ControlShape::typeId);
	CHECK_MSTATUS_AND_RETURN_IT(stat);

	return MS::kSuccess;
}

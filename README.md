## What is controlShape?
**controlShape** is a Maya shape node that can be attached to whole meshes or their faces.

![controlShape](https://user-images.githubusercontent.com/9614751/173179759-a1854ed7-91c7-45f3-8696-ff3ac486184c.gif)

https://www.youtube.com/watch?v=8aVKpmMS_aA

## Features
+ Static and dynamic meshes.
+ Scale by a normal.
+ Color and opacity control.

## How to use
Compile the plugin for the Maya version you want using Visual Studio. No dependencies required.

Use the following code to create or parent controlShapes.
```python
import pymel.core as pm

def makeControlShape():
    ls = pm.ls(sl=True)
    faces = pm.ls(sl=True, fl=True, type="float3")
    
    sh = pm.createNode("controlShape")
    if faces:
        mesh = faces[0].node()
        mesh.outMesh >> sh.mesh
        sh.meshInWorldSpace.set(True)        
    
        indices = [f.index() for f in faces]        
        sh.meshFaces.set(indices, type="Int32Array")
        sh.meshMatrix.set(mesh.getParent().wm.get())
        
    elif ls:
        pm.connectAttr(ls[0].outMesh, sh.mesh)
        pm.disconnectAttr(ls[0].outMesh, sh.mesh)
        sh.getParent().setMatrix(ls[0].wm.get())
        
def parentControlShape():
    ls = pm.ls(sl=True)
    if ls and len(ls) == 2:
        controlShape, control = ls
        
        for sh in controlShape.listRelatives(s=True, type="controlShape"):
            pm.parent(sh, control, r=True, s=True)
            sh.rename(control+"Shape")
            
            if not sh.meshInWorldSpace.get():
                sh.meshMatrix.set(controlShape.getMatrix() * control.wim.get())
                                
        pm.delete(controlShape)
    else:
        pm.warning("Select controlShape and transform")    

#makeControlShape() # select mesh/mesh faces
#parentControlShape() # select controlShape transform and a control you want to parent to.
```
`makeControlShape` can be applied to both whole meshes and mesh faces. In the first case there will be a static mesh control created. The second one case creates a dynamic deformable shape control. 

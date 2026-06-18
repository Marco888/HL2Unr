class EdMeshTest extends Actor;

struct FTestStr
{
	var() name Bone;
	var() rotator R;
	var() int Space;
	var() float Alpha;
};
var() array<FTestStr> TestAr;

event DrawEditorSelection( Canvas C )
{
	local int i;
	
	for( i=0; i<TestAr.Size(); ++i )
	{
		SetBoneRotation(GetBoneIndex(string(TestAr[i].Bone)),TestAr[i].R,TestAr[i].Alpha,TestAr[i].Space);
	}
}

defaultproperties
{
	bStatic=true
	bEditorSelectRender=true
	DrawType=DT_Mesh
	Mesh=BarneyM
	AnimSequence="shootgun"
}
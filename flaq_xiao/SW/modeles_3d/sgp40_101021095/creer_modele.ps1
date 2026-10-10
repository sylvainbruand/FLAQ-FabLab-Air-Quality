$ErrorActionPreference = 'Stop'
$dll = 'C:\Program Files\SOLIDWORKS Corp\SOLIDWORKS\SolidWorks.Interop.sldworks.dll'
Add-Type -Path $dll
Add-Type -ReferencedAssemblies $dll -TypeDefinition @'
using System;
using System.IO;
using SolidWorks.Interop.sldworks;
public class SGP40Model {
 static IModelDoc2 doc;
 static IFeature plane;
 static ISketchManager sk;
 static double M(double v) { return v / 1000.0; }
 static void Begin() {
  doc.ClearSelection2(true); plane.Select2(false,0); sk.InsertSketch(true); sk.AddToDB=true;
 }
 static void Rect(double x,double y,double w,double h) {
  sk.CreateCornerRectangle(M(x),M(y),0,M(x+w),M(y+h),0);
 }
 static void Circle(double x,double y,double r) { sk.CreateCircleByRadius(M(x),M(y),0,M(r)); }
 static IFeature Extrude(string name,double z,double height,bool merge) {
  sk.AddToDB=false; sk.InsertSketch(true);
  IFeature f=doc.FeatureManager.FeatureExtrusion2(true,false,false,0,0,M(height),0,false,false,false,false,0,0,false,false,false,false,merge,true,true,z==0?0:3,M(z),false);
  if(f==null) throw new Exception("Extrusion failed: "+name);
  f.Name=name; return f;
 }
 static void Cut(string name,double z,double depth) {
  sk.AddToDB=false; sk.InsertSketch(true);
  IFeature f=doc.FeatureManager.FeatureCut3(true,false,false,0,0,M(depth),0,false,false,false,false,0,0,false,false,false,false,false,true,true,false,false,false,3,M(z),false);
  if(f==null) throw new Exception("Cut failed: "+name);
  f.Name=name;
 }

 static void Wire(double x1,double y1,double x2,double y2,double angle) {
  if(Math.Abs(angle)<0.000001) {sk.CreateLine(M(x1),M(y1),0,M(x2),M(y2),0);return;}
  double a=angle*Math.PI/180,dx=x2-x1,dy=y2-y1;
  double cx=(x1+x2)/2-dy/(2*Math.Tan(a/2)),cy=(y1+y2)/2+dx/(2*Math.Tan(a/2));
  sk.CreateArc(M(cx),M(cy),0,M(x1),M(y1),0,M(x2),M(y2),0,(short)(angle>0?1:-1));
 }
 public static void Run(string folder) {
  ISldWorks sw=(ISldWorks)Activator.CreateInstance(Type.GetTypeFromProgID("SldWorks.Application"));

  doc=(IModelDoc2)sw.NewDocument(@"C:\ProgramData\SOLIDWORKS\SOLIDWORKS 2025\templates\Pièce.PRTDOT",0,0,0);
  if(doc==null) throw new Exception("NewDocument failed");
  for(IFeature f=(IFeature)doc.FirstFeature(); f!=null; f=(IFeature)f.GetNextFeature()) {
   if(f.GetTypeName2()=="RefPlane") { plane=f; break; }
  }
  if(plane==null) throw new Exception("Reference plane missing");
  sk=doc.SketchManager;

  Begin();
  Wire(-19.835,10.0,-20.635,9.2,90);
  Wire(-20.635,-9.2,-19.835,-10.0,90);
  Wire(18.564999999999998,-10.0,19.365,-9.2,90);
  Wire(19.365,-9.2,19.365,-2.1,0);
  Wire(19.365,2.1,19.365,9.2,0);
  Wire(19.365,9.2,18.564999999999998,10.0,90);
  Wire(18.564999999999998,10.0,12.065,10.0,0);
  Wire(12.065,10.0,11.765,9.7,90);
  Wire(-19.835,-10.0,-12.735,-10.0,0);
  Wire(-12.735,-10.0,-12.535,-10.2,-90);
  Wire(-8.535,-10.0,-8.735,-10.2,90);
  Wire(-8.735,-10.2,-12.535,-10.2,-167.981988);
  Wire(-19.835,10.0,-12.834999999999999,10.0,0);
  Wire(-12.834999999999999,10.0,-12.535,10.3,90);
  Wire(-8.435,10.0,6.665,10.0,0);
  Wire(6.665,10.0,6.965,9.7,-90);
  Wire(-8.435,10.0,-8.735,10.3,-90);
  Wire(-12.535,10.3,-8.735,10.3,-162.054747);
  Wire(6.965,9.7,11.765,9.7,165.749967);
  Wire(19.365,-2.1,19.564999999999998,-1.9,-90);
  Wire(19.365,2.1,19.564999999999998,1.9,90);
  Wire(19.564999999999998,1.9,19.564999999999998,-1.9,-167.981988);
  Wire(6.665,-10.0,6.965,-9.7,90);
  Wire(12.065,-10.0,11.765,-9.7,-90);
  Wire(11.765,-9.7,6.965,-9.7,165.749967);
  Wire(6.665,-10.0,-8.535,-10.0,0);
  Wire(12.065,-10.0,18.564999999999998,-10.0,0);
  Wire(-20.635,2.7,-20.335,2.4,90);
  Wire(-20.635,-2.7,-20.335,-2.4,-90);
  Wire(-20.335,-2.4,-20.335,2.4,165.749967);
  Wire(-20.635,-2.7,-20.635,-9.2,0);
  Wire(-20.635,2.7,-20.635,9.2,0);
  Circle(-10.635,-10,1.1);
  Circle(-10.635,10,1.1);
  Circle(19.365,0,1.1);
  Wire(5.301,3.754,10.539,3.754,0);
  Wire(10.539,3.754,10.539,-3.246,0);
  Wire(10.539,-3.246,5.301,-3.246,0);
  Wire(5.301,-3.246,5.301,-4.246,0);
  Wire(5.301,-4.246,11.539,-4.246,0);
  Wire(11.539,-4.246,11.539,4.754,0);
  Wire(11.539,4.754,5.301,4.754,0);
  Wire(5.301,4.754,5.301,3.754,0);
  Extrude("PCB_contour_percages_et_isolation_Eagle_ep_ESTIMEE_1_6",0,1.6,false);

  Begin();Rect(-19.984840625,-6.0898375,9.2,12.2);Extrude("Grove_base_H_ESTIMEE",1.6,.7,false);
  Begin();Rect(-12.284840625,-6.0898375,1.5,12.2);Extrude("Grove_dos",2.3,3.7,false);
  Begin();Rect(-19.984840625,-6.0898375,7.7,.7);Rect(-19.984840625,5.4101625,7.7,.7);Extrude("Grove_flancs",2.3,3.7,false);
  Begin();Rect(-19.984840625,-6.0898375,9.2,12.2);Extrude("Grove_toit",6,.7,false);
  Begin();Rect(-20,-3.39,1.1,6.8);Cut("Encoche_Grove",6.7,.7);
  Begin();for(int i=0;i<4;i++)Rect(-17.8,-3.24+i*2,5.52,.5);Extrude("Broches_horizontales_Grove",3.4,.5,false);
  Begin();Rect(6.654,-.966,2.44,2.44);Extrude("SGP40_2_44x2_44x0_85",1.65,.85,false);
  Begin();Circle(8.174,.554,.4);Cut("Ouverture_SGP40_diam0_8_profondeur_ESTIMEE",2.5,.3);

 Begin();Rect(1.486,2.521,1.6,0.8);Extrude("C1_position_Eagle",1.65,0.55,false);
 Begin();Rect(1.486,4.299,1.6,0.8);Extrude("C2_position_Eagle",1.65,0.55,false);
 Begin();Rect(-6.149,-0.396,2.9,1.3);Extrude("U2_position_Eagle",1.65,1,false);
 Begin();Rect(-5.988,3.01,0.8,1.6);Extrude("C3_position_Eagle",1.65,0.55,false);
 Begin();Rect(-4.21,3.01,0.8,1.6);Extrude("C4_position_Eagle",1.65,0.55,false);
 Begin();Rect(-5.988,-4.229,0.8,1.6);Extrude("C5_position_Eagle",1.65,0.55,false);
 Begin();Rect(-4.21,-4.229,0.8,1.6);Extrude("C6_position_Eagle",1.65,0.55,false);
 Begin();Rect(-2.72,-0.396,2.9,1.3);Extrude("Q7_position_Eagle",1.65,1,false);
 Begin();Rect(0.836,-0.396,2.9,1.3);Extrude("Q8_position_Eagle",1.65,1,false);
 Begin();Rect(1.486,-3.067,1.6,0.8);Extrude("R20_position_Eagle",1.65,0.55,false);
 Begin();Rect(-2.07,-3.067,1.6,0.8);Extrude("R21_position_Eagle",1.65,0.55,false);
 Begin();Rect(-0.654,3.01,0.8,1.6);Extrude("R22_position_Eagle",1.65,0.55,false);
 Begin();Rect(-2.432,3.01,0.8,1.6);Extrude("R23_position_Eagle",1.65,0.55,false);
 Begin();Rect(2.6035,2.4765,0.889,0.889);Extrude("Pad_C1_0",1.6,.05,false);
 Begin();Rect(1.0795,2.4765,0.889,0.889);Extrude("Pad_C1_1",1.6,.05,false);
 Begin();Rect(2.6035,4.2545,0.889,0.889);Extrude("Pad_C2_0",1.6,.05,false);
 Begin();Rect(1.0795,4.2545,0.889,0.889);Extrude("Pad_C2_1",1.6,.05,false);
 Begin();Rect(-12.05484063,-3.4948375,2.74,1.01);Extrude("Pad_J1_0",1.6,.05,false);
 Begin();Rect(-12.05484063,-1.4948375,2.74,1.01);Extrude("Pad_J1_1",1.6,.05,false);
 Begin();Rect(-12.05484063,0.5051625,2.74,1.01);Extrude("Pad_J1_2",1.6,.05,false);
 Begin();Rect(-12.05484063,2.5051625,2.74,1.01);Extrude("Pad_J1_3",1.6,.05,false);
 Begin();Rect(-19.68484062,-6.1398375,3,1.5);Extrude("Pad_J1_4",1.6,.05,false);
 Begin();Rect(-19.68484062,4.6601625,3,1.5);Extrude("Pad_J1_5",1.6,.05,false);
 Begin();Rect(-5.207,0.6985,1.016,1.143);Extrude("Pad_U2_0",1.6,.05,false);
 Begin();Rect(-4.318,-1.3335,1.016,1.143);Extrude("Pad_U2_1",1.6,.05,false);
 Begin();Rect(-6.096,-1.3335,1.016,1.143);Extrude("Pad_U2_2",1.6,.05,false);
 Begin();Rect(-6.0325,2.6035,0.889,0.889);Extrude("Pad_C3_0",1.6,.05,false);
 Begin();Rect(-6.0325,4.1275,0.889,0.889);Extrude("Pad_C3_1",1.6,.05,false);
 Begin();Rect(-4.2545,2.6035,0.889,0.889);Extrude("Pad_C4_0",1.6,.05,false);
 Begin();Rect(-4.2545,4.1275,0.889,0.889);Extrude("Pad_C4_1",1.6,.05,false);
 Begin();Rect(-6.0325,-3.1115,0.889,0.889);Extrude("Pad_C5_0",1.6,.05,false);
 Begin();Rect(-6.0325,-4.6355,0.889,0.889);Extrude("Pad_C5_1",1.6,.05,false);
 Begin();Rect(-4.2545,-3.1115,0.889,0.889);Extrude("Pad_C6_0",1.6,.05,false);
 Begin();Rect(-4.2545,-4.6355,0.889,0.889);Extrude("Pad_C6_1",1.6,.05,false);
 Begin();Rect(-1.778,0.6985,1.016,1.143);Extrude("Pad_Q7_0",1.6,.05,false);
 Begin();Rect(-0.889,-1.3335,1.016,1.143);Extrude("Pad_Q7_1",1.6,.05,false);
 Begin();Rect(-2.667,-1.3335,1.016,1.143);Extrude("Pad_Q7_2",1.6,.05,false);
 Begin();Rect(1.778,0.6985,1.016,1.143);Extrude("Pad_Q8_0",1.6,.05,false);
 Begin();Rect(2.667,-1.3335,1.016,1.143);Extrude("Pad_Q8_1",1.6,.05,false);
 Begin();Rect(0.889,-1.3335,1.016,1.143);Extrude("Pad_Q8_2",1.6,.05,false);
 Begin();Rect(1.0795,-3.1115,0.889,0.889);Extrude("Pad_R20_0",1.6,.05,false);
 Begin();Rect(2.6035,-3.1115,0.889,0.889);Extrude("Pad_R20_1",1.6,.05,false);
 Begin();Rect(-2.4765,-3.1115,0.889,0.889);Extrude("Pad_R21_0",1.6,.05,false);
 Begin();Rect(-0.9525,-3.1115,0.889,0.889);Extrude("Pad_R21_1",1.6,.05,false);
 Begin();Rect(-0.6985,4.1275,0.889,0.889);Extrude("Pad_R22_0",1.6,.05,false);
 Begin();Rect(-0.6985,2.6035,0.889,0.889);Extrude("Pad_R22_1",1.6,.05,false);
 Begin();Rect(-2.4765,4.1275,0.889,0.889);Extrude("Pad_R23_0",1.6,.05,false);
 Begin();Rect(-2.4765,2.6035,0.889,0.889);Extrude("Pad_R23_1",1.6,.05,false);
 Begin();Rect(6.874,-1.271,0.4,0.65);Extrude("Pad_U1_0",1.6,.05,false);
 Begin();Rect(7.674,-1.271,0.4,0.65);Extrude("Pad_U1_1",1.6,.05,false);
 Begin();Rect(8.474,-1.271,0.4,0.65);Extrude("Pad_U1_2",1.6,.05,false);
 Begin();Rect(8.474,1.129,0.4,0.65);Extrude("Pad_U1_3",1.6,.05,false);
 Begin();Rect(7.674,1.129,0.4,0.65);Extrude("Pad_U1_4",1.6,.05,false);
 Begin();Rect(6.874,1.129,0.4,0.65);Extrude("Pad_U1_5",1.6,.05,false);
 Begin();Rect(7.124,-0.271,1.5,1.05);Extrude("Pad_U1_6",1.6,.05,false);

  object[] bodies=(object[])((IPartDoc)doc).GetBodies2(0,false);
  int pcb=0,sensor=0;
  foreach(IBody2 b in bodies){double[] bb=(double[])b.GetBodyBox();double dx=bb[3]-bb[0],dy=bb[4]-bb[1],dz=bb[5]-bb[2];double[] c;
   if(dx>.039){c=new double[]{.02,.35,.49,1,.7,.2,.2,0,0};b.Name="Carte_Grove_101021095";pcb++;}
   else if(bb[0]<-.01 && dz>.0006){c=new double[]{.92,.91,.86,1,.7,.1,.1,0,0};}
   else if(dz<.0001 || (dx>.005 && dy<.001)){c=new double[]{.75,.73,.67,1,.6,.6,.4,0,0};}
   else {c=new double[]{.15,.14,.14,1,.7,.1,.1,0,0};if(bb[0]>.006 && dx>.002){sensor++;b.Name="Capteur_SGP40";}}
   b.MaterialPropertyValues2=c;
  }
  if(pcb!=1 || sensor!=1 || bodies.Length<50)throw new Exception("Geometry check failed; bodies="+bodies.Length);
  doc.ForceRebuild3(false);doc.ShowNamedView2("*Isometric",7);doc.ViewZoomtofit2();
  int err=0,warn=0;string target=Path.Combine(folder,"Grove_SGP40_101021095.SLDPRT");
  if(!doc.Extension.SaveAs(target,0,1,null,ref err,ref warn))throw new Exception("Save failed "+err);
  Console.WriteLine("Saved "+target+"; bodies="+bodies.Length+"; warnings="+warn);
  doc.SaveBMP(Path.Combine(folder,"apercu.bmp"),1200,900);
 }
}
'@
[SGP40Model]::Run($PSScriptRoot)

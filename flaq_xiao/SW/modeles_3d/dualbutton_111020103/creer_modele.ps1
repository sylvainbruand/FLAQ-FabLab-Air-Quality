$ErrorActionPreference = 'Stop'
$dll = 'C:\Program Files\SOLIDWORKS Corp\SOLIDWORKS\SolidWorks.Interop.sldworks.dll'
Add-Type -Path $dll
Add-Type -ReferencedAssemblies $dll -TypeDefinition @'
using System;
using System.IO;
using SolidWorks.Interop.sldworks;
public class DualButtonModel {
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
 Wire(0.612,19.652,-0.188,18.852,90);
 Wire(-0.188,0.452,0.612,-0.348,90);
 Wire(39.012,-0.348,39.812,0.452,90);
 Wire(39.812,0.452,39.812,7.552,0);
 Wire(39.812,11.752,39.812,18.852,0);
 Wire(39.812,18.852,39.012,19.652,90);
 Wire(39.012,19.652,32.512,19.652,0);
 Wire(32.512,19.652,32.212,19.352,90);
 Wire(0.612,-0.348,7.712,-0.348,0);
 Wire(7.712,-0.348,7.912,-0.548,-90);
 Wire(11.912,-0.348,11.712,-0.548,90);
 Wire(11.712,-0.548,7.912,-0.548,-167.981988);
 Wire(0.612,19.652,7.612,19.652,0);
 Wire(7.612,19.652,7.912,19.952,90);
 Wire(12.012,19.652,27.112,19.652,0);
 Wire(27.112,19.652,27.412,19.352,-90);
 Wire(12.012,19.652,11.712,19.952,-90);
 Wire(7.912,19.952,11.712,19.952,-162.054747);
 Wire(27.412,19.352,32.212,19.352,165.749967);
 Wire(39.812,7.552,40.012,7.752,-90);
 Wire(39.812,11.752,40.012,11.552,90);
 Wire(40.012,11.552,40.012,7.752,-167.981988);
 Wire(27.112,-0.348,27.412,-0.048,90);
 Wire(32.512,-0.348,32.212,-0.048,-90);
 Wire(32.212,-0.048,27.412,-0.048,165.749967);
 Wire(27.112,-0.348,11.912,-0.348,0);
 Wire(32.512,-0.348,39.012,-0.348,0);
 Wire(-0.188,12.352,0.112,12.052,90);
 Wire(-0.188,6.952,0.112,7.252,-90);
 Wire(0.112,7.252,0.112,12.052,165.749967);
 Wire(-0.188,6.952,-0.188,0.452,0);
 Wire(-0.188,12.352,-0.188,18.852,0);
 Circle(9.812,-0.348,1.1);
 Circle(9.812,19.652,1.1);
 Circle(39.812,9.652,1.1);
 Extrude("PCB_contour_et_percages_Eagle_ep_ESTIMEE1_6",0,1.6,false);
 Begin();
 Wire(15.144,3.652,5.684,3.652,0);
 Wire(5.684,3.652,4.414,4.922,0);
 Wire(4.414,4.922,4.414,14.382,0);
 Wire(4.414,14.382,5.684,15.652,0);
 Wire(5.684,15.652,15.144,15.652,0);
 Wire(15.144,15.652,16.414,14.382,0);
 Wire(16.414,14.382,16.414,4.922,0);
 Wire(16.414,4.922,15.144,3.652,0);
 Extrude("K2_socle12x12_H_ESTIMEE",1.6,3.5,false);
 Begin();Rect(5.013999999999999,4.252,10.8,10.8);Extrude("K2_plaque",5.1,.3,false);
 Begin();Circle(10.414,9.652,2);Extrude("K2_poussoir_H7_5_empreinte",5.4,3.7,false);
 Begin();Circle(10.414,9.652,6.1);Circle(10.414,9.652,2.1);Extrude("K2_capuchon_bord_ESTIME",7.4,.7,false);
 Begin();Circle(10.414,9.652,5.6);Extrude("K2_capuchon_ESTIME",8.1,3.4,false);
 Begin();
 Wire(34.194,3.652,24.734,3.652,0);
 Wire(24.734,3.652,23.464,4.922,0);
 Wire(23.464,4.922,23.464,14.382,0);
 Wire(23.464,14.382,24.734,15.652,0);
 Wire(24.734,15.652,34.194,15.652,0);
 Wire(34.194,15.652,35.464,14.382,0);
 Wire(35.464,14.382,35.464,4.922,0);
 Wire(35.464,4.922,34.194,3.652,0);
 Extrude("K1_socle12x12_H_ESTIMEE",1.6,3.5,false);
 Begin();Rect(24.064,4.252,10.8,10.8);Extrude("K1_plaque",5.1,.3,false);
 Begin();Circle(29.464,9.652,2);Extrude("K1_poussoir_H7_5_empreinte",5.4,3.7,false);
 Begin();Circle(29.464,9.652,6.1);Circle(29.464,9.652,2.1);Extrude("K1_capuchon_bord_ESTIME",7.4,.7,false);
 Begin();Circle(29.464,9.652,5.6);Extrude("K1_capuchon_ESTIME",8.1,3.4,false);

 // shortcut: cap dimensions and connector height estimated from photo; verify a real part for panel cutouts.
 Begin();Rect(1.468,3.552,9.2,12.2);Extrude("Grove_dessous_base",-.7,.7,false);
 Begin();Rect(9.168,3.552,1.5,12.2);Extrude("Grove_dessous_dos",-4.4,3.7,false);
 Begin();Rect(1.468,3.552,7.7,.7);Rect(1.468,15.052,7.7,.7);Extrude("Grove_dessous_flancs",-4.4,3.7,false);
 Begin();Rect(1.468,3.552,9.2,12.2);Extrude("Grove_dessous_toit_H_ESTIMEE5_1",-5.1,.7,false);
 Begin();for(int i=0;i<4;i++)Rect(3.7,6.402+2*i,5.468,.5);Extrude("Contacts_Grove_dessous",-3.1,.5,false);

 Begin();Rect(16.999000000000002,5.042,.8,1.6);Extrude("C1_dessous_position_Eagle",-.6,.55,false);
 Begin();Rect(16.999000000000002,6.566,.8,1.6);Extrude("C2_dessous_position_Eagle",-.6,.55,false);
 Begin();Rect(16.999000000000002,9.741,.8,1.6);Extrude("R3_dessous_position_Eagle",-.6,.55,false);
 Begin();Rect(16.999000000000002,11.264999999999999,.8,1.6);Extrude("R4_dessous_position_Eagle",-.6,.55,false);
 Begin();Rect(16.999000000000002,8.216999999999999,.8,1.6);Extrude("R5_dessous_position_Eagle",-.6,.55,false);
 Begin();Rect(16.999000000000002,12.789,.8,1.6);Extrude("R6_dessous_position_Eagle",-.6,.55,false);
 Begin();Rect(17.7165,5.3975,0.889,0.889);Extrude("Pad_C1_0",-0.05,.05,false);
 Begin();Rect(16.1925,5.3975,0.889,0.889);Extrude("Pad_C1_1",-0.05,.05,false);
 Begin();Rect(17.7165,6.9215,0.889,0.889);Extrude("Pad_C2_0",-0.05,.05,false);
 Begin();Rect(16.1925,6.9215,0.889,0.889);Extrude("Pad_C2_1",-0.05,.05,false);
 Begin();Rect(21.089,11.402,2,1.5);Extrude("Pad_K1_0",1.6,.05,false);
 Begin();Rect(21.089,6.402,2,1.5);Extrude("Pad_K1_1",1.6,.05,false);
 Begin();Rect(35.839,6.402,2,1.5);Extrude("Pad_K1_2",1.6,.05,false);
 Begin();Rect(35.839,11.402,2,1.5);Extrude("Pad_K1_3",1.6,.05,false);
 Begin();Rect(2.039,11.402,2,1.5);Extrude("Pad_K2_0",1.6,.05,false);
 Begin();Rect(2.039,6.402,2,1.5);Extrude("Pad_K2_1",1.6,.05,false);
 Begin();Rect(16.789,6.402,2,1.5);Extrude("Pad_K2_2",1.6,.05,false);
 Begin();Rect(16.789,11.402,2,1.5);Extrude("Pad_K2_3",1.6,.05,false);
 Begin();Rect(16.1925,10.0965,0.889,0.889);Extrude("Pad_R3_0",-0.05,.05,false);
 Begin();Rect(17.7165,10.0965,0.889,0.889);Extrude("Pad_R3_1",-0.05,.05,false);
 Begin();Rect(17.7165,11.6205,0.889,0.889);Extrude("Pad_R4_0",-0.05,.05,false);
 Begin();Rect(16.1925,11.6205,0.889,0.889);Extrude("Pad_R4_1",-0.05,.05,false);
 Begin();Rect(16.1925,8.5725,0.889,0.889);Extrude("Pad_R5_0",-0.05,.05,false);
 Begin();Rect(17.7165,8.5725,0.889,0.889);Extrude("Pad_R5_1",-0.05,.05,false);
 Begin();Rect(17.7165,13.1445,0.889,0.889);Extrude("Pad_R6_0",-0.05,.05,false);
 Begin();Rect(16.1925,13.1445,0.889,0.889);Extrude("Pad_R6_1",-0.05,.05,false);
 Begin();Rect(9.398,12.147,2.74,1.01);Extrude("Pad_J1_0",-0.05,.05,false);
 Begin();Rect(9.398,10.147,2.74,1.01);Extrude("Pad_J1_1",-0.05,.05,false);
 Begin();Rect(9.398,8.147,2.74,1.01);Extrude("Pad_J1_2",-0.05,.05,false);
 Begin();Rect(9.398,6.147,2.74,1.01);Extrude("Pad_J1_3",-0.05,.05,false);
 Begin();Rect(1.768,14.302,3,1.5);Extrude("Pad_J1_4",-0.05,.05,false);
 Begin();Rect(1.768,3.502,3,1.5);Extrude("Pad_J1_5",-0.05,.05,false);

 object[] bodies=(object[])((IPartDoc)doc).GetBodies2(0,false);int pcb=0,caps=0;
 foreach(IBody2 b in bodies){double[] bb=(double[])b.GetBodyBox();double dx=bb[3]-bb[0],dy=bb[4]-bb[1],dz=bb[5]-bb[2];double[] c;
  if(dx>.039){c=new double[]{.02,.35,.49,1,.7,.2,.2,0,0};b.Name="PCB_Grove_111020103";pcb++;}
  else if(bb[2]>.0073 && dx>.01){c=bb[0]<.02?new double[]{.08,.7,.12,1,.7,.3,.2,0,0}:new double[]{.85,.04,.03,1,.7,.3,.2,0,0};if(dz>.003)caps++;}
  else if(bb[2]<-.0005 && dx>.001 && dy>.0006 && bb[0]<.011){c=new double[]{.92,.91,.86,1,.7,.1,.1,0,0};}
  else if(dz<.0004 || (bb[2]<-.002 && dy<.001)){c=new double[]{.75,.73,.67,1,.6,.6,.4,0,0};}
  else c=new double[]{.13,.12,.13,1,.7,.1,.1,0,0};
  b.MaterialPropertyValues2=c;
 }
 if(pcb!=1 || caps!=2 || bodies.Length<40)throw new Exception("Geometry check failed: "+bodies.Length);
 doc.ForceRebuild3(false);doc.ShowNamedView2("*Isometric",7);doc.ViewZoomtofit2();
 int err=0,warn=0;string target=Path.Combine(folder,"Grove_Dual_Button_111020103.SLDPRT");
 if(!doc.Extension.SaveAs(target,0,1,null,ref err,ref warn))throw new Exception("Save failed "+err);
 Console.WriteLine("Saved "+target+"; bodies="+bodies.Length+"; warnings="+warn);
 doc.SaveBMP(Path.Combine(folder,"apercu.bmp"),1200,900);
 }
}
'@
[DualButtonModel]::Run($PSScriptRoot)

$ErrorActionPreference = 'Stop'
$dll = 'C:\Program Files\SOLIDWORKS Corp\SOLIDWORKS\SolidWorks.Interop.sldworks.dll'
Add-Type -Path $dll
Add-Type -ReferencedAssemblies $dll -TypeDefinition @'
using System;
using System.IO;
using SolidWorks.Interop.sldworks;
public class OLEDModel {
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
 Wire(-12.2045,10,-19.2045,10,0);
 Wire(-19.2045,10,-20.0045,9.2,90);
 Wire(-20.0045,9.2,-20.0045,2.2,0);
 Wire(-20.0045,2.2,-19.9045,2.1,90);
 Wire(-19.9045,2.1,-17.9045,0.1,-90);
 Wire(-17.9045,0.1,-17.9045,-0.1,0);
 Wire(-17.9045,-0.1,-19.9045,-2.1,-90);
 Wire(-19.9045,-2.1,-20.0045,-2.2,90);
 Wire(-20.0045,-2.2,-20.0045,-9.2,0);
 Wire(-20.0045,-9.2,-19.2045,-10,90);
 Wire(-19.2045,-10,-12.2045,-10,0);
 Wire(-12.2045,-10,-12.1045,-10.1,-90);
 Wire(-12.1045,-10.1,-10.1045,-12.1,90);
 Wire(-10.1045,-12.1,-9.9045,-12.1,0);
 Wire(-9.9045,-12.1,-7.9045,-10.1,90);
 Wire(-7.9045,-10.1,-7.8045,-10,-90);
 Wire(-7.8045,-10,7.7955,-10,0);
 Wire(7.7955,-10,7.8955,-9.9,90);
 Wire(7.8955,-9.9,9.8955,-7.9,-90);
 Wire(9.8955,-7.9,10.0955,-7.9,0);
 Wire(10.0955,-7.9,12.0955,-9.9,-90);
 Wire(12.0955,-9.9,12.1955,-10,90);
 Wire(12.1955,-10,19.1955,-10,0);
 Wire(19.1955,-10,19.9955,-9.2,90);
 Wire(19.9955,-9.2,19.9955,-2.2,0);
 Wire(19.9955,-2.2,20.0955,-2.1,-90);
 Wire(20.0955,-2.1,22.0955,-0.1,90);
 Wire(22.0955,-0.1,22.0955,0.1,0);
 Wire(22.0955,0.1,20.0955,2.1,90);
 Wire(20.0955,2.1,19.9955,2.2,-90);
 Wire(19.9955,2.2,19.9955,9.2,0);
 Wire(19.9955,9.2,19.1955,10,90);
 Wire(19.1955,10,12.1955,10,0);
 Wire(12.1955,10,12.0955,9.9,90);
 Wire(12.0955,9.9,10.0955,7.9,-90);
 Wire(10.0955,7.9,9.8955,7.9,0);
 Wire(9.8955,7.9,7.8955,9.9,-90);
 Wire(7.8955,9.9,7.7955,10,90);
 Wire(7.7955,10,-7.8045,10,0);
 Wire(-7.8045,10,-7.9045,10.1,-90);
 Wire(-7.9045,10.1,-9.9045,12.1,90);
 Wire(-9.9045,12.1,-10.1045,12.1,0);
 Wire(-10.1045,12.1,-12.1045,10.1,90);
 Wire(-12.1045,10.1,-12.2045,10,-90);
 Circle(-10.0045,-10,1.1);
 Circle(-10.0045,10,1.1);
 Circle(19.9955,0,1.1);
 Circle(-16.51,-3,.35);
 Circle(-16.51,-1,.35);
 Circle(-16.51,1,.35);
 Circle(-16.51,3,.35);

 Extrude("PCB_source_officielle_V1_0_V1_1_NON_VERIFIEE",0,1.6,false);
 // shortcut: published source is v1.0; verify v1.1 geometry and mounting height against the user's physical module.
 Begin();Rect(-18.71,-5,5,10);Extrude("Grove_H_ESTIMEE8_4",1.6,8.4,false);
 Begin();Rect(-18.01,-4.3,3.6,8.6);Cut("Cavite_Grove",10,7.3);
 Begin();Rect(-14.51,-3.2,1,1.4);Rect(-14.51,1.8,1,1.4);Cut("Detrompeurs_Grove",10,3.2);
 Begin();for(int i=0;i<4;i++)Rect(-16.76,-3.25+2*i,.5,.5);Extrude("Contacts_Grove",-.8,9.4,false);
 Begin();Rect(-4.52,-6.5,21.74,13);Extrude("Support_ecran_H_ESTIMEE1",1.6,1,false);
 Begin();Rect(-6.02,-8.45,24.74,16.9);Extrude("Dalle_OLED_24_74x16_9x1_42",2.6,1.4,false);
 Begin();Rect(-6.02,-8.45,24.74,16.9);Rect(-4.52,-3.91,21.74,10.86);Extrude("Bordure_dalle",4,.02,false);
 Begin();Rect(-4.52,-3.91,21.74,10.86);Extrude("Zone_active_21_74x10_86",4,.02,false);
 Begin();Rect(2.35,-8.8,8,.35);Extrude("Nappe_repliee_simplifiee",1.8,2.2,false);

 Begin();Rect(-7.798,2.925,1,0.5);Extrude("R1_position_Eagle",1.65,0.4,false);
 Begin();Rect(-7.798,0.3596,1,0.5);Extrude("R2_position_Eagle",1.65,0.4,false);
 Begin();Rect(-11.9599,-1.774,1,0.5);Extrude("R3_position_Eagle",1.65,0.4,false);
 Begin();Rect(-10.4904,-1.774,1,0.5);Extrude("R4_position_Eagle",1.65,0.4,false);
 Begin();Rect(-10.3345,2.5545,2,1.25);Extrude("Q3_position_Eagle",1.65,1,false);
 Begin();Rect(-10.3345,0.0145,2,1.25);Extrude("Q4_position_Eagle",1.65,1,false);
 Begin();Rect(-11.1955,4.646,1.3,2.9);Extrude("U1_position_Eagle",1.65,1,false);
 Begin();Rect(-12.2255,0.235,1.6,0.8);Extrude("C1_position_Eagle",1.65,0.6,false);
 Begin();Rect(-8.098,5.95,1.6,0.8);Extrude("C2_position_Eagle",1.65,0.6,false);
 Begin();Rect(-7.7771,-1.774,1,0.5);Extrude("C3_position_Eagle",1.65,0.4,false);
 Begin();Rect(-7.7644,-3.425,1,0.5);Extrude("C4_position_Eagle",1.65,0.4,false);
 Begin();Rect(-12.23,-6.5468,1.6,0.8);Extrude("C5_position_Eagle",1.65,0.6,false);
 Begin();Rect(-10.4695,-3.425,1,0.5);Extrude("R5_position_Eagle",1.65,0.4,false);
 Begin();Rect(-10.2806,-6.7468,0.8,1.2);Extrude("D1_position_Eagle",1.65,0.6,false);
 Begin();Rect(-8.124,-5.1736,0.5,1);Extrude("R8_position_Eagle",1.65,0.4,false);
 Begin();Rect(-8.274,-6.5658,0.8,1.6);Extrude("C6_position_Eagle",1.65,0.6,false);
 Begin();Rect(-8.274,-7.912,0.8,1.6);Extrude("C7_position_Eagle",1.65,0.6,false);
 Begin();Rect(-9.1233,-1.774,1,0.5);Extrude("R6_position_Eagle",1.65,0.4,false);
 Begin();Rect(-11.93,2.925,1,0.5);Extrude("C8_position_Eagle",1.65,0.4,false);
 Begin();Rect(-8.9455,6.354,1,0.5);Extrude("C9_position_Eagle",1.65,0.4,false);
 Begin();Rect(-11.93,-3.425,1,0.5);Extrude("R7_position_Eagle",1.65,0.4,false);
 Begin();Rect(-9.1106,-3.425,1,0.5);Extrude("R9_position_Eagle",1.65,0.4,false);
 Begin();Rect(-7.552,2.5995,0.508,0.385);Extrude("Pad_R1_0",1.6,.05,false);
 Begin();Rect(-7.552,3.3655,0.508,0.385);Extrude("Pad_R1_1",1.6,.05,false);
 Begin();Rect(-7.552,0.8001,0.508,0.385);Extrude("Pad_R2_0",1.6,.05,false);
 Begin();Rect(-7.552,0.0341,0.508,0.385);Extrude("Pad_R2_1",1.6,.05,false);
 Begin();Rect(-11.7139,-1.3335,0.508,0.385);Extrude("Pad_R3_0",1.6,.05,false);
 Begin();Rect(-11.7139,-2.0995,0.508,0.385);Extrude("Pad_R3_1",1.6,.05,false);
 Begin();Rect(-10.2444,-1.3335,0.508,0.385);Extrude("Pad_R4_0",1.6,.05,false);
 Begin();Rect(-10.2444,-2.0995,0.508,0.385);Extrude("Pad_R4_1",1.6,.05,false);
 Begin();Rect(-8.8265,2.227,0.762,0.635);Extrude("Pad_Q3_0",1.6,.05,false);
 Begin();Rect(-8.8265,3.497,0.762,0.635);Extrude("Pad_Q3_1",1.6,.05,false);
 Begin();Rect(-10.6045,2.862,0.762,0.635);Extrude("Pad_Q3_2",1.6,.05,false);
 Begin();Rect(-8.8265,-0.313,0.762,0.635);Extrude("Pad_Q4_0",1.6,.05,false);
 Begin();Rect(-8.8265,0.957,0.762,0.635);Extrude("Pad_Q4_1",1.6,.05,false);
 Begin();Rect(-10.6045,0.322,0.762,0.635);Extrude("Pad_Q4_2",1.6,.05,false);
 Begin();Rect(-12.133,5.588,1.143,1.016);Extrude("Pad_U1_0",1.6,.05,false);
 Begin();Rect(-10.101,6.477,1.143,1.016);Extrude("Pad_U1_1",1.6,.05,false);
 Begin();Rect(-10.101,4.699,1.143,1.016);Extrude("Pad_U1_2",1.6,.05,false);
 Begin();Rect(-11.8065,0.889,0.762,0.696);Extrude("Pad_C1_0",1.6,.05,false);
 Begin();Rect(-11.8065,-0.315,0.762,0.696);Extrude("Pad_C1_1",1.6,.05,false);
 Begin();Rect(-7.679,6.604,0.762,0.696);Extrude("Pad_C2_0",1.6,.05,false);
 Begin();Rect(-7.679,5.4,0.762,0.696);Extrude("Pad_C2_1",1.6,.05,false);
 Begin();Rect(-7.5311,-2.0995,0.508,0.385);Extrude("Pad_C3_0",1.6,.05,false);
 Begin();Rect(-7.5311,-1.3335,0.508,0.385);Extrude("Pad_C3_1",1.6,.05,false);
 Begin();Rect(-7.5184,-3.7505,0.508,0.385);Extrude("Pad_C4_0",1.6,.05,false);
 Begin();Rect(-7.5184,-2.9845,0.508,0.385);Extrude("Pad_C4_1",1.6,.05,false);
 Begin();Rect(-11.811,-7.0968,0.762,0.696);Extrude("Pad_C5_0",1.6,.05,false);
 Begin();Rect(-11.811,-5.8928,0.762,0.696);Extrude("Pad_C5_1",1.6,.05,false);
 Begin();Rect(-10.2235,-3.7505,0.508,0.385);Extrude("Pad_R5_0",1.6,.05,false);
 Begin();Rect(-10.2235,-2.9845,0.508,0.385);Extrude("Pad_R5_1",1.6,.05,false);
 Begin();Rect(-10.2306,-7.3278,0.7,0.762);Extrude("Pad_D1_0",1.6,.05,false);
 Begin();Rect(-10.2306,-5.7278,0.7,0.762);Extrude("Pad_D1_1",1.6,.05,false);
 Begin();Rect(-7.6835,-4.9276,0.385,0.508);Extrude("Pad_R8_0",1.6,.05,false);
 Begin();Rect(-8.4495,-4.9276,0.385,0.508);Extrude("Pad_R8_1",1.6,.05,false);
 Begin();Rect(-8.824,-6.1468,0.696,0.762);Extrude("Pad_C6_0",1.6,.05,false);
 Begin();Rect(-7.62,-6.1468,0.696,0.762);Extrude("Pad_C6_1",1.6,.05,false);
 Begin();Rect(-8.824,-7.493,0.696,0.762);Extrude("Pad_C7_0",1.6,.05,false);
 Begin();Rect(-7.62,-7.493,0.696,0.762);Extrude("Pad_C7_1",1.6,.05,false);
 Begin();Rect(-17.018,-8.128,1.016,1.016);Extrude("Pad_JP1_0",1.6,.05,false);
 Begin();Rect(-18.288,-8.128,1.016,1.016);Extrude("Pad_JP1_1",1.6,.05,false);
 Begin();Rect(-15.748,-8.128,1.016,1.016);Extrude("Pad_JP1_2",1.6,.05,false);
 Begin();Rect(-8.8773,-2.0995,0.508,0.385);Extrude("Pad_R6_0",1.6,.05,false);
 Begin();Rect(-8.8773,-1.3335,0.508,0.385);Extrude("Pad_R6_1",1.6,.05,false);
 Begin();Rect(-11.684,3.3655,0.508,0.385);Extrude("Pad_C8_0",1.6,.05,false);
 Begin();Rect(-11.684,2.5995,0.508,0.385);Extrude("Pad_C8_1",1.6,.05,false);
 Begin();Rect(-8.6995,6.0285,0.508,0.385);Extrude("Pad_C9_0",1.6,.05,false);
 Begin();Rect(-8.6995,6.7945,0.508,0.385);Extrude("Pad_C9_1",1.6,.05,false);
 Begin();Rect(-11.684,-2.9845,0.508,0.385);Extrude("Pad_R7_0",1.6,.05,false);
 Begin();Rect(-11.684,-3.7505,0.508,0.385);Extrude("Pad_R7_1",1.6,.05,false);
 Begin();Rect(-8.8646,-2.9845,0.508,0.385);Extrude("Pad_R9_0",1.6,.05,false);
 Begin();Rect(-8.8646,-3.7505,0.508,0.385);Extrude("Pad_R9_1",1.6,.05,false);

 object[] bodies=(object[])((IPartDoc)doc).GetBodies2(0,false);int pcb=0,screen=0;
 foreach(IBody2 b in bodies){double[] bb=(double[])b.GetBodyBox();double dx=bb[3]-bb[0],dy=bb[4]-bb[1],dz=bb[5]-bb[2];double[] c;
  if(dx>.039){c=new double[]{.02,.35,.49,1,.7,.2,.2,0,0};b.Name="PCB_V1_0_compatibilite_V1_1_non_verifiee";pcb++;}
  else if(dx>.021 && dx<.022 && bb[2]>.0039){c=new double[]{.045,.055,.09,1,.7,.4,.3,0,0};b.Name="Zone_active_OLED";screen++;}
  else if(dx>.024){c=new double[]{.10,.08,.12,1,.7,.4,.3,0,0};}
  else if(bb[0]<-.018 && dz>.008){c=new double[]{.92,.91,.86,1,.7,.1,.1,0,0};}
  else if(dz<.0002 || dz>.005){c=new double[]{.75,.73,.67,1,.6,.6,.4,0,0};}
  else c=new double[]{.13,.12,.13,1,.7,.1,.1,0,0};
  b.MaterialPropertyValues2=c;
 }
 if(pcb!=1 || screen!=1 || bodies.Length<60)throw new Exception("Geometry check failed: "+bodies.Length);
 doc.ForceRebuild3(false);doc.ShowNamedView2("*Isometric",7);doc.ViewZoomtofit2();
 int err=0,warn=0;string target=Path.Combine(folder,"Grove_OLED_SSD1315_PROVISOIRE_base_v1_0.SLDPRT");
 if(!doc.Extension.SaveAs(target,0,1,null,ref err,ref warn))throw new Exception("Save failed "+err);
 Console.WriteLine("Saved "+target+"; bodies="+bodies.Length+"; warnings="+warn);
 doc.SaveBMP(Path.Combine(folder,"apercu.bmp"),1200,900);
 }
}
'@
[OLEDModel]::Run($PSScriptRoot)

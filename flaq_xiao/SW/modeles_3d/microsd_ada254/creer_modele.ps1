$ErrorActionPreference = 'Stop'
$dll = 'C:\Program Files\SOLIDWORKS Corp\SOLIDWORKS\SolidWorks.Interop.sldworks.dll'
Add-Type -Path $dll
Add-Type -ReferencedAssemblies $dll -TypeDefinition @'
using System;
using System.IO;
using SolidWorks.Interop.sldworks;
public class MicroSDModel {
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
 Wire(31.75,1.905,29.845,0,-90);
 Wire(29.845,0,1.905,0,0);
 Wire(1.905,0,0,1.905,-90);
 Wire(0,1.905,0,23.495,0);
 Wire(0,23.495,1.905,25.4,-90);
 Wire(1.905,25.4,29.845,25.4,0);
 Wire(29.845,25.4,31.75,23.495,-90);
 Wire(31.75,23.495,31.75,1.905,0);
 Circle(2.54,22.86,1.1);
 Circle(2.54,2.54,1.1);
 Circle(22.86,22.86,1.1);
 Circle(22.86,2.54,1.1);
 Circle(30.48,3.683,0.508);
 Circle(30.48,6.223,0.508);
 Circle(30.48,8.763,0.508);
 Circle(30.48,11.303,0.508);
 Circle(30.48,13.843,0.508);
 Circle(30.48,16.383,0.508);
 Circle(30.48,18.923,0.508);
 Circle(30.48,21.463,0.508);

 Extrude("PCB_Adafruit_Eagle_31_75x25_4_ep_ESTIMEE1_6",0,1.6,false);
 // shortcut: socket section, height and stamped details estimated from product photo; verify hardware for tight enclosures.
 Begin();Rect(.608,5.558,15.1,14);Extrude("MicroSD_base_H_ESTIMEE",1.6,.4,false);
 Begin();Rect(.608,5.558,15.1,.65);Rect(.608,18.908,15.1,.65);Extrude("MicroSD_flancs",2,1.8,false);
 Begin();Rect(15.058,6.208,.65,12.7);Extrude("MicroSD_fond",2,1.8,false);
 Begin();Rect(.608,5.558,15.1,14);Extrude("MicroSD_capot_metallique",3.8,.2,false);
 Begin();Rect(3.7,10.4,1.6,3.5);Rect(3.7,14.8,1.6,3.5);Rect(2.6,6.1,5,1);Cut("Fenetres_capot_simplifiees",4,.2);
 Begin();for(int i=0;i<4;i++)Rect(10,10.8+1.25*i,5.8,.16);Cut("Fentes_capot_simplifiees",4,.2);
 Begin();for(int i=0;i<8;i++)Rect(4.5,10.208+1.1*i,5,.35);Extrude("Contacts_microSD_simplifies",2,.12,false);

 Begin();Rect(19.132,8.766,3.9,9.9);Extrude("IC1_position_Eagle",1.65,1.75,false);
 Begin();Rect(14.313,1.598,1.6,2.9);Extrude("IC2_position_Eagle",1.65,1.1,false);
 Begin();Rect(7.798,2.375,3.2,1.6);Extrude("C1_position_Eagle",1.65,1.8,false);
 Begin();Rect(17.288,5.852,2,1.25);Extrude("C2_position_Eagle",1.65,0.8,false);
 Begin();Rect(6.995,20.082,1.25,2);Extrude("ACT_position_Eagle",1.65,0.8,false);
 Begin();Rect(10.811,20.457,2,1.25);Extrude("R1_position_Eagle",1.65,0.55,false);
 Begin();Rect(16.9926,17.8308,2.032,0.6604);Extrude("Pad_IC1_0",1.6,0.12,false);
 Begin();Rect(23.1394,17.8308,2.032,0.6604);Extrude("Pad_IC1_1",1.6,0.12,false);
 Begin();Rect(16.9926,16.5608,2.032,0.6604);Extrude("Pad_IC1_2",1.6,0.12,false);
 Begin();Rect(16.9926,15.2908,2.032,0.6604);Extrude("Pad_IC1_3",1.6,0.12,false);
 Begin();Rect(23.1394,16.5608,2.032,0.6604);Extrude("Pad_IC1_4",1.6,0.12,false);
 Begin();Rect(23.1394,15.2908,2.032,0.6604);Extrude("Pad_IC1_5",1.6,0.12,false);
 Begin();Rect(16.9926,14.0208,2.032,0.6604);Extrude("Pad_IC1_6",1.6,0.12,false);
 Begin();Rect(23.1394,14.0208,2.032,0.6604);Extrude("Pad_IC1_7",1.6,0.12,false);
 Begin();Rect(16.9926,12.7508,2.032,0.6604);Extrude("Pad_IC1_8",1.6,0.12,false);
 Begin();Rect(23.1394,12.7508,2.032,0.6604);Extrude("Pad_IC1_9",1.6,0.12,false);
 Begin();Rect(16.9926,11.4808,2.032,0.6604);Extrude("Pad_IC1_10",1.6,0.12,false);
 Begin();Rect(16.9926,10.2108,2.032,0.6604);Extrude("Pad_IC1_11",1.6,0.12,false);
 Begin();Rect(23.1394,11.4808,2.032,0.6604);Extrude("Pad_IC1_12",1.6,0.12,false);
 Begin();Rect(23.1394,10.2108,2.032,0.6604);Extrude("Pad_IC1_13",1.6,0.12,false);
 Begin();Rect(16.9926,8.9408,2.032,0.6604);Extrude("Pad_IC1_14",1.6,0.12,false);
 Begin();Rect(23.1394,8.9408,2.032,0.6604);Extrude("Pad_IC1_15",1.6,0.12,false);
 Begin();Rect(1.258,18.458,1.9,1.4);Extrude("Pad_X1_0",1.6,0.05,false);
 Begin();Rect(0.358,5.258,1.9,1.4);Extrude("Pad_X1_1",1.6,0.05,false);
 Begin();Rect(14.608,16.658,1.4,1.8);Extrude("Pad_X1_2",1.6,0.05,false);
 Begin();Rect(14.608,10.658,1.4,1.8);Extrude("Pad_X1_3",1.6,0.05,false);
 Begin();Rect(4.358,17.908,1.5,0.7);Extrude("Pad_X1_4",1.6,0.05,false);
 Begin();Rect(4.358,16.808,1.5,0.7);Extrude("Pad_X1_5",1.6,0.05,false);
 Begin();Rect(3.958,15.708,1.5,0.7);Extrude("Pad_X1_6",1.6,0.05,false);
 Begin();Rect(4.358,14.608,1.5,0.7);Extrude("Pad_X1_7",1.6,0.05,false);
 Begin();Rect(3.958,13.508,1.5,0.7);Extrude("Pad_X1_8",1.6,0.05,false);
 Begin();Rect(4.358,12.408,1.5,0.7);Extrude("Pad_X1_9",1.6,0.05,false);
 Begin();Rect(4.758,11.308,1.5,0.7);Extrude("Pad_X1_10",1.6,0.05,false);
 Begin();Rect(4.358,10.208,1.5,0.7);Extrude("Pad_X1_11",1.6,0.05,false);
 Begin();Rect(15.813,1.823,1.2,0.55);Extrude("Pad_IC2_0",1.6,0.12,false);
 Begin();Rect(15.813,2.773,1.2,0.55);Extrude("Pad_IC2_1",1.6,0.12,false);
 Begin();Rect(15.813,3.723,1.2,0.55);Extrude("Pad_IC2_2",1.6,0.12,false);
 Begin();Rect(13.213,3.723,1.2,0.55);Extrude("Pad_IC2_3",1.6,0.12,false);
 Begin();Rect(13.213,1.823,1.2,0.55);Extrude("Pad_IC2_4",1.6,0.12,false);
 Begin();Rect(9.798,2.425,1.95,1.5);Extrude("Pad_C1_0",1.6,0.05,false);
 Begin();Rect(7.048,2.425,1.95,1.5);Extrude("Pad_C1_1",1.6,0.05,false);
 Begin();Rect(16.738,5.777,1.1,1.4);Extrude("Pad_C2_0",1.6,0.05,false);
 Begin();Rect(18.738,5.777,1.1,1.4);Extrude("Pad_C2_1",1.6,0.05,false);
 Begin();Circle(30.48,3.683,0.889);Circle(30.48,3.683,0.508);Extrude("Anneau_JP1_0",1.6,.05,false);
 Begin();Circle(30.48,6.223,0.889);Circle(30.48,6.223,0.508);Extrude("Anneau_JP1_1",1.6,.05,false);
 Begin();Circle(30.48,8.763,0.889);Circle(30.48,8.763,0.508);Extrude("Anneau_JP1_2",1.6,.05,false);
 Begin();Circle(30.48,11.303,0.889);Circle(30.48,11.303,0.508);Extrude("Anneau_JP1_3",1.6,.05,false);
 Begin();Circle(30.48,13.843,0.889);Circle(30.48,13.843,0.508);Extrude("Anneau_JP1_4",1.6,.05,false);
 Begin();Circle(30.48,16.383,0.889);Circle(30.48,16.383,0.508);Extrude("Anneau_JP1_5",1.6,.05,false);
 Begin();Circle(30.48,18.923,0.889);Circle(30.48,18.923,0.508);Extrude("Anneau_JP1_6",1.6,.05,false);
 Begin();Circle(30.48,21.463,0.889);Circle(30.48,21.463,0.508);Extrude("Anneau_JP1_7",1.6,.05,false);
 Begin();Rect(8.07,20.482,1.2,1.2);Extrude("Pad_ACT_0",1.6,0.05,false);
 Begin();Rect(5.97,20.482,1.2,1.2);Extrude("Pad_ACT_1",1.6,0.05,false);
 Begin();Rect(12.261,20.382,1.1,1.4);Extrude("Pad_R1_0",1.6,0.05,false);
 Begin();Rect(10.261,20.382,1.1,1.4);Extrude("Pad_R1_1",1.6,0.05,false);

 object[] bodies=(object[])((IPartDoc)doc).GetBodies2(0,false);int pcb=0,capot=0;
 foreach(IBody2 b in bodies){double[] bb=(double[])b.GetBodyBox();double dx=bb[3]-bb[0],dy=bb[4]-bb[1],dz=bb[5]-bb[2];double[] c;
  if(dx>.03){c=new double[]{.02,.27,.60,1,.7,.2,.2,0,0};b.Name="PCB_Adafruit_ADA254";pcb++;}
  else if(dx>.014 && bb[2]>.0037){c=new double[]{.78,.79,.80,1,.6,.7,.4,0,0};b.Name="Capot_microSD";capot++;}
  else if(dz<.0003){c=new double[]{.78,.68,.36,1,.6,.6,.4,0,0};}
  else if(bb[0]>.007 && bb[0]<.010 && bb[1]<.004){c=new double[]{.64,.49,.27,1,.7,.1,.1,0,0};}
  else c=new double[]{.13,.13,.15,1,.7,.1,.1,0,0};
  b.MaterialPropertyValues2=c;
 }
 if(pcb!=1 || capot!=1 || bodies.Length<45)throw new Exception("Geometry check failed: "+bodies.Length);
 doc.ForceRebuild3(false);doc.ShowNamedView2("*Isometric",7);doc.ViewZoomtofit2();
 int err=0,warn=0;string target=Path.Combine(folder,"Adafruit_MicroSD_ADA254.SLDPRT");
 if(!doc.Extension.SaveAs(target,0,1,null,ref err,ref warn))throw new Exception("Save failed "+err);
 Console.WriteLine("Saved "+target+"; bodies="+bodies.Length+"; warnings="+warn);
 doc.SaveBMP(Path.Combine(folder,"apercu.bmp"),1200,900);
 }
}
'@
[MicroSDModel]::Run($PSScriptRoot)

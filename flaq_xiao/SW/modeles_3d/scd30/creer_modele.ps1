$ErrorActionPreference = 'Stop'
$dll = 'C:\Program Files\SOLIDWORKS Corp\SOLIDWORKS\SolidWorks.Interop.sldworks.dll'
Add-Type -Path $dll
Add-Type -ReferencedAssemblies $dll -TypeDefinition @'
using System;
using System.IO;
using SolidWorks.Interop.sldworks;
public class ScdModel {
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
  double a=angle*Math.PI/180, dx=x2-x1,dy=y2-y1;
  double cx=(x1+x2)/2-dy/(2*Math.Tan(a/2)),cy=(y1+y2)/2+dx/(2*Math.Tan(a/2));
  sk.CreateArc(M(cx),M(cy),0,M(x1),M(y1),0,M(x2),M(y2),0,(short)(angle>0?1:-1));
 }
 public static void Run(string folder) {
  ISldWorks sw=(ISldWorks)Activator.CreateInstance(Type.GetTypeFromProgID("SldWorks.Application"));

  string step=Path.Combine(folder,@"sensirion_cad\Sensirion_CO2_Sensors_SCD30_POD_STEP_File.step");
  int ie=0; IModelDoc2 src=(IModelDoc2)sw.LoadFile4(step,"r",null,ref ie);
  if(src==null) throw new Exception("Sensirion import failed "+ie);
  object[] official=(object[])((IPartDoc)src).GetBodies2(0,false);
  if(official==null || official.Length<20) throw new Exception("Sensirion solids missing");
  doc=(IModelDoc2)sw.NewDocument(@"C:\ProgramData\SOLIDWORKS\SOLIDWORKS 2025\templates\Pièce.PRTDOT",0,0,0);
  if(doc==null) throw new Exception("NewDocument failed");
  for(IFeature f=(IFeature)doc.FirstFeature(); f!=null; f=(IFeature)f.GetNextFeature()) {
   if(f.GetTypeName2()=="RefPlane") { plane=f; break; }
  }
  if(plane==null) throw new Exception("Reference plane missing");
  sk=doc.SketchManager;
  Begin();
  Wire(-30,-19.2,-29.2,-20,90);
  Wire(-22.2,-20,-29.2,-20,0);
  Wire(-30,-12.2,-30,-19.2,0);
  Wire(-22.2,-20,-22.1,-20.1,-90);
  Wire(-17.9,-20.1,-17.8,-20,-78.865599);
  Wire(-22.1,-20.1,-20.1,-22.1,90);
  Wire(-30,-7.8,-29.9,-7.9,90);
  Wire(-27.9,-9.9,-29.9,-7.9,90);
  Wire(-30,-12.2,-29.9,-12.1,-90);
  Wire(-27.9,-10.1,-29.9,-12.1,-90);
  Wire(-27.9,-10.1,-27.9,-9.9,0);
  Wire(-19.9,-22.1,-17.9,-20.1,90);
  Wire(-19.9,-22.1,-20.1,-22.1,0);
  Wire(17.8,-20,2.2,-20,0);
  Wire(-2.2,-20,-2.1,-19.9,90);
  Wire(-0.1,-17.9,-2.1,-19.9,90);
  Wire(2.2,-20,2.1,-19.9,-90);
  Wire(0.1,-17.9,2.1,-19.9,-90);
  Wire(0.1,-17.9,-0.1,-17.9,0);
  Wire(-30,19.2,-29.2,20,-90);
  Wire(-30,12.2,-30,19.2,0);
  Wire(-30,-7.8,-30,7.8,0);
  Wire(-30,7.8,-30.1,7.9,90);
  Wire(-30.1,7.9,-32.1,9.9,-90);
  Wire(-32.1,9.9,-32.1,10.1,0);
  Wire(-32.1,10.1,-30.1,12.1,-90);
  Wire(-30.1,12.1,-30,12.2,90);
  Wire(-22.2,20,-22.1,19.9,-90);
  Wire(-22.1,19.9,-20,17.9,92.794362);
  Wire(-17.9,19.9,-17.8,20,-79.236906);
  Wire(-17.9,19.9,-19.9,17.9,-90);
  Wire(-19.9,17.9,-20,17.9,0);
  Wire(-29.2,20,-22.2,20,0);
  Wire(-2.2,20,-17.8,20,0);
  Wire(-2.2,20,-2.1,20.1,90);
  Wire(-2.1,20.1,-0.1,22.1,-90);
  Wire(-0.1,22.1,0.1,22.1,0);
  Wire(0.1,22.1,2.1,20.1,-90);
  Wire(2.1,20.1,2.2,20,90);
  Wire(30,19.2,29.2,20,90);
  Wire(30,12.2,30,19.2,0);
  Wire(2.2,20,17.8,20,0);
  Wire(30,7.8,30.1,7.9,-90);
  Wire(30.1,12.1,30,12.2,-78.865599);
  Wire(30.1,7.9,32.1,9.9,90);
  Wire(17.8,20,17.9,19.9,-90);
  Wire(17.9,19.9,20,17.9,92.794362);
  Wire(22.1,19.9,22.2,20,-79.236906);
  Wire(22.1,19.9,20.1,17.9,-90);
  Wire(20.1,17.9,20,17.9,0);
  Wire(32.1,10.1,30.1,12.1,90);
  Wire(32.1,10.1,32.1,9.9,0);
  Wire(30,7.8,30,-7.8,0);
  Wire(29.2,20,22.2,20,0);
  Wire(29.2,-20,30,-19.2,90);
  Wire(22.2,-20,29.2,-20,0);
  Wire(17.8,-20,17.9,-20.1,-90);
  Wire(22.1,-20.1,22.2,-20,-78.865599);
  Wire(17.9,-20.1,19.9,-22.1,90);
  Wire(30,-7.8,29.9,-7.9,-90);
  Wire(29.9,-7.9,27.9,-10,92.794362);
  Wire(29.9,-12.1,30,-12.2,-79.236906);
  Wire(29.9,-12.1,27.9,-10.1,-90);
  Wire(27.9,-10.1,27.9,-10,0);
  Wire(20.1,-22.1,22.1,-20.1,90);
  Wire(20.1,-22.1,19.9,-22.1,0);
  Wire(30,-19.2,30,-12.2,0);
  Wire(-2.2,-20,-17.8,-20,0);
  Circle(-0.017,20.015,1.1);
  Circle(-30.005,10.02,1.1);
  Circle(29.985,9.983,1.1);
  Circle(19.98,-20.005,1.1);
  Circle(-19.983,-20.015,1.1);
  Extrude("Carte_Grove_contour_Eagle_ep_ESTIMEE_1_6",0,1.6,false);
  Begin(); Circle(26.67,0.1778,1.05);Circle(-14.4272,0.1778,1.05);
  for(int i=0;i<4;i++) Circle(-10.2328+2.54*i,-9.9742,0.4445); Cut("Percages_capot_et_connecteur_Eagle",1.6,1.7);
  Begin(); Rect(-29.209,-5.872,7.7,12.2); Extrude("Connecteur_Grove_H_ESTIMEE",1.6,5,false);
  Begin(); Rect(-29.5,-5.072,6.5,10.6); Cut("Ouverture_Grove",5.9,3.5);
  Begin(); for(int i=0;i<4;i++) Rect(-28.5,-3.022+2*i,5.5,0.5);Extrude("Contacts_Grove",3,0.5,false);
  Begin(); Rect(-11.5028,-11.2442,10.16,2.54);Extrude("Support_4P_ESTIME",1.6,2.5,false);
  Begin(); for(int i=0;i<4;i++) Rect(-10.55+2.54*i,-10.29,0.64,0.64);Extrude("Broches_liaison_ESTIMEES",1.6,3.6,false);
  IMathUtility mu=(IMathUtility)sw.GetMathUtility();
  MathTransform tr=(MathTransform)mu.CreateTransform(new double[]{-1,0,0,0,0,1,0,1,0,0.0075544,0.003399,0.0036,1,0,0,0});
  int nb=0;
  foreach(IBody2 ob in official) {
   IBody2 copy=(IBody2)ob.Copy(); if(!copy.ApplyTransform(tr))throw new Exception("Transform failed");
   IFeature f=(IFeature)((IPartDoc)doc).CreateFeatureFromBody3(copy,false,0);
   if(f==null) throw new Exception("Body insert failed"); f.Name="SCD30_Sensirion_"+(++nb);
  }
  // Seeed silkscreen cover contour. Heights and visual openings estimated.
  Begin();
  double[,] pp={{-15.59614,-2.53785},{-15.59614,2.88506},{-14.38402,4.72333},{-14.38402,11.6736},{-12.38402,13.6736},{24.61598,13.6736},{26.61598,11.6736},{26.61598,4.75618},{27.81598,2.92315},{27.81598,-2.57594},{26.61598,-4.40897},{26.61598,-11.3264},{24.61598,-13.3264},{-12.38402,-13.3264},{-14.38402,-11.3264},{-14.38402,-4.37612}};
  for(int i=0;i<16;i++) Wire(pp[i,0],pp[i,1],pp[(i+1)%16,0],pp[(i+1)%16,1],0);
  Rect(-12,-11.6,36.5,23.4);
  Extrude("Parois_capot_contour_Seeed_H_ESTIMEE",2.6,15.4,false);
  Begin(); for(int i=0;i<16;i++) Wire(pp[i,0],pp[i,1],pp[(i+1)%16,0],pp[(i+1)%16,1],0);
  Circle(-14.4272,0.1778,1.05);Circle(26.67,0.1778,1.05);
  Circle(-5,-4,2.7);Circle(3,5,2.7);Circle(11,-4,2.7);Rect(20,4,3.6,3.6);
  Extrude("Couvercle_ouvertures_ESTIMEES",18,1,false);
  Begin(); Circle(-14.4272,0.1778,1.7);Circle(-14.4272,0.1778,0.7);Circle(26.67,0.1778,1.7);Circle(26.67,0.1778,0.7);
  Extrude("Vis_simplifiees",18.7,0.3,false);
  IPartDoc part=(IPartDoc)doc; object[] bodies=(object[])part.GetBodies2(0,false);
  if(bodies.Length<official.Length+10) throw new Exception("Missing output solids");
  foreach(IBody2 b2 in bodies) {
   double[] bb=(double[])b2.GetBodyBox();double dx=bb[3]-bb[0],dy=bb[4]-bb[1],dz=bb[5]-bb[2];double[] c;
   if(dx>0.06) c=new double[]{0.015,0.30,0.47,1,0.7,0.2,0.2,0,0};
   else if(dx>0.04) c=new double[]{0.12,0.13,0.15,1,0.7,0.2,0.2,0,0};
   else if(dx>0.03) c=new double[]{0.25,0.3,0.3,1,0.6,0.3,0.2,0,0};
   else if(dy>0.011 && bb[0]<-0.02) c=new double[]{0.93,0.92,0.86,1,0.7,0.2,0.2,0,0};
   else c=new double[]{0.48,0.49,0.5,1,0.6,0.6,0.4,0,0};
   b2.MaterialPropertyValues2=c;
  }
  doc.ForceRebuild3(false); doc.ShowNamedView2("*Isometric",7);doc.ViewZoomtofit2();
  int errors=0,warnings=0;
  string target=Path.Combine(folder,"Grove_SCD30.SLDPRT");
  if(!doc.Extension.SaveAs(target,0,1,null,ref errors,ref warnings)) throw new Exception("Save SLDPRT error "+errors);
  Console.WriteLine("SLDPRT: "+target+" warnings="+warnings);
  Console.WriteLine("SLDPRT solids="+bodies.Length);
  bool bmp=doc.SaveBMP(Path.Combine(folder,"apercu.bmp"),1400,800);
  Console.WriteLine("Preview="+bmp);
 }
}
'@
[ScdModel]::Run($PSScriptRoot)

$ErrorActionPreference = 'Stop'
$dll = 'C:\Program Files\SOLIDWORKS Corp\SOLIDWORKS\SolidWorks.Interop.sldworks.dll'
Add-Type -Path $dll
Add-Type -ReferencedAssemblies $dll -TypeDefinition @'
using System;
using System.IO;
using SolidWorks.Interop.sldworks;
public class HM3301Model {
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
 public static void Run(string folder) {
  ISldWorks sw=(ISldWorks)Activator.CreateInstance(Type.GetTypeFromProgID("SldWorks.Application"));

  sw.CloseDoc("Grove_HM3301.SLDPRT");
  doc=(IModelDoc2)sw.NewDocument(@"C:\ProgramData\SOLIDWORKS\SOLIDWORKS 2025\templates\Pièce.PRTDOT",0,0,0);
  if(doc==null) throw new Exception("NewDocument failed");
  for(IFeature f=(IFeature)doc.FirstFeature(); f!=null; f=(IFeature)f.GetNextFeature()) {
   if(f.GetTypeName2()=="RefPlane") { plane=f; break; }
  }
  if(plane==null) throw new Exception("Reference plane missing");
  sk=doc.SketchManager;
  // PCB contour and hole centres from Seeed Eagle v1.0. Z values are estimates.
  Begin();
  sk.CreateLine(M(-37),M(-20),0,M(37),M(-20),0);
  sk.CreateArc(M(37),M(-17),0,M(37),M(-20),0,M(40),M(-17),0,1);
  sk.CreateLine(M(40),M(-17),0,M(40),M(17),0);
  sk.CreateArc(M(37),M(17),0,M(40),M(17),0,M(37),M(20),0,1);
  sk.CreateLine(M(37),M(20),0,M(-37),M(20),0);
  sk.CreateArc(M(-37),M(17),0,M(-37),M(20),0,M(-40),M(17),0,1);
  sk.CreateLine(M(-40),M(17),0,M(-40),M(-17),0);
  sk.CreateArc(M(-37),M(-17),0,M(-40),M(-17),0,M(-37),M(-20),0,1);
  foreach(double x in new double[]{-36,36}) foreach(double y in new double[]{-16,16}) Circle(x,y,1.6);
  Circle(-23.622,-17.018,1.2); Circle(12.573,17.145,1.2);
  Extrude("PCB_Eagle_80x40_R3_ep_ESTIMEE_1_6",0,1.6,false);
  Begin(); Rect(-25.089,-18.487,39.4,37.4); Extrude("HM3301_corps_40x38x15_Z_estime",3.6,14.6,false);
  Begin(); Rect(-25.389,-18.787,40,38); Extrude("Capot_superieur_ep_simplifiee",18.2,0.4,false);
  Begin(); Rect(-25.389,-18.787,40,0.3); Rect(-25.389,18.913,40,0.3);
  Extrude("Flancs_capot_simplifies",4.6,13.6,false);
  Begin(); Rect(-25.389,-18.487,0.3,37.4); Rect(14.311,-18.487,0.3,37.4);
  Extrude("Faces_capot_simplifiees",4.6,13.6,false);
  Begin(); Circle(-23.622,-17.018,2.8); Circle(12.573,17.145,2.8);
  Cut("Degagements_vis_ESTIMES",18.6,12.8);
  // Six sector-shaped grille openings, visual geometry inferred from official photo.
  Begin();
  for(int i=0;i<6;i++) {
   double cx=-14.0,cy=-6.5,a=(i*60+6)*Math.PI/180,b=(i*60+54)*Math.PI/180;
   double px=cx+7.2*Math.Cos(a),py=cy+7.2*Math.Sin(a);
   for(int j=1;j<=8;j++) {double t=a+(b-a)*j/8,xx=cx+7.2*Math.Cos(t),yy=cy+7.2*Math.Sin(t);sk.CreateLine(M(px),M(py),0,M(xx),M(yy),0);px=xx;py=yy;}
   double qx=cx+5.2*Math.Cos(b),qy=cy+5.2*Math.Sin(b);sk.CreateLine(M(px),M(py),0,M(qx),M(qy),0);px=qx;py=qy;
   for(int j=1;j<=8;j++) {double t=b-(b-a)*j/8,xx=cx+5.2*Math.Cos(t),yy=cy+5.2*Math.Sin(t);sk.CreateLine(M(px),M(py),0,M(xx),M(yy),0);px=xx;py=yy;}
   sk.CreateLine(M(px),M(py),0,M(cx+7.2*Math.Cos(a)),M(cy+7.2*Math.Sin(a)),0);
  }
  Cut("Grille_ventilateur_ESTIMEE",18.6,1.5);
  Begin(); Circle(-14,-6.5,4.5); Extrude("Etiquette_ventilateur_simplifiee",18.6,0.03,false);
  Begin(); Rect(-20,-19.0,3.5,0.8); Rect(6,-19.0,3.5,0.8); Rect(14.0,-12,0.8,4); Rect(14.0,8,0.8,4);
  Cut("Fentes_laterales_ESTIMEES",16.2,1.2);
  Begin(); Rect(-20,-19.0,3.5,0.8); Rect(6,-19.0,3.5,0.8); Cut("Fentes_basses_ESTIMEES",8.2,1.2);
  Begin(); Rect(-39.5276,-6.1,7.7,12.2); Extrude("Grove_position_Eagle_hauteur_ESTIMEE",1.6,5.0,false);
  Begin(); Rect(-40,-5.3,6.5,10.6); Cut("Ouverture_Grove",5.9,3.5);
  Begin(); for(int p=0;p<4;p++) Rect(-38.5,-3.25+p*2,5.5,0.5); Extrude("Broches_Grove",3.0,0.5,false);
  Begin(); Rect(35.969,-7.5,3.5,15); Extrude("Connecteur_8P_position_Eagle_H_ESTIMEE",1.6,4,false);
  Begin(); Rect(36.569,-6.8,2.3,13.6); Cut("Cavite_8P_simplifiee",5.6,2.7);
  Begin(); foreach(double[] p in new double[][] {new double[]{-23.622,-17.018},new double[]{12.573,17.145}}) {Circle(p[0],p[1],2);Circle(p[0],p[1],1.2);}
  Extrude("Tetes_vis_simplifiees",5.8,1.1,false);
  Begin(); Rect(20.682000000000002,-3.4669999999999996,0.8,1.6); Extrude("Composant_Eagle_1",1.6,0.7,false);
  Begin(); Rect(20.082,-1.127,2,2); Extrude("Composant_Eagle_2",1.6,0.7,false);
  Begin(); Rect(16.726,-1.127,1.6,2); Extrude("Composant_Eagle_3",1.6,0.7,false);
  Begin(); Rect(17.107,-3.956,1.6,0.8); Extrude("Composant_Eagle_4",1.6,0.7,false);
  Begin(); Rect(15.837,-3.956,1.6,0.8); Extrude("Composant_Eagle_5",1.6,0.7,false);
  Begin(); Rect(20.682000000000002,1.3589999999999998,0.8,1.6); Extrude("Composant_Eagle_6",1.6,0.7,false);
  Begin(); Rect(20.682000000000002,-4.737,0.8,1.6); Extrude("Composant_Eagle_7",1.6,0.7,false);
  Begin(); Rect(20.682000000000002,2.7560000000000002,0.8,1.6); Extrude("Composant_Eagle_8",1.6,0.7,false);
  Begin(); Rect(23.315,-5.245,2.9,1.6); Extrude("Composant_Eagle_9",1.6,0.7,false);
  Begin(); Rect(23.765,-9.055000000000001,2,1.6); Extrude("Composant_Eagle_10",1.6,0.7,false);
  Begin(); Rect(20.682000000000002,-6.007,0.8,1.6); Extrude("Composant_Eagle_11",1.6,0.7,false);
  Begin(); Rect(20.682000000000002,-9.055000000000001,0.8,1.6); Extrude("Composant_Eagle_12",1.6,0.7,false);
  Begin(); Rect(20.682000000000002,-7.6579999999999995,0.8,1.6); Extrude("Composant_Eagle_13",1.6,0.7,false);
  Begin(); Rect(31.584999999999997,-8.528,1.6,0.8); Extrude("Composant_Eagle_14",1.6,0.7,false);
  Begin(); Rect(30.061,-8.528,1.6,0.8); Extrude("Composant_Eagle_15",1.6,0.7,false);
  Begin(); Rect(27.061,-5.445,1.25,2); Extrude("Composant_Eagle_16",1.6,0.7,false);
  Begin(); Rect(28.537,-4.845000000000001,1.6,0.8); Extrude("Composant_Eagle_17",1.6,0.7,false);
  Begin(); Rect(29.445,-3.4669999999999996,0.8,1.6); Extrude("Composant_Eagle_18",1.6,0.7,false);
  Begin(); Rect(27.267,-8.528,1.6,0.8); Extrude("Composant_Eagle_19",1.6,0.7,false);
  Begin(); Rect(20.433,5.115,1.25,2); Extrude("Composant_Eagle_20",1.6,0.7,false);
  Begin(); Rect(26.363,5.6770000000000005,2.9,1.6); Extrude("Composant_Eagle_21",1.6,0.7,false);
  Begin(); Rect(24.599999999999998,6.077,1.6,0.8); Extrude("Composant_Eagle_22",1.6,0.7,false);
  Begin(); Rect(23.076,6.077,1.6,0.8); Extrude("Composant_Eagle_23",1.6,0.7,false);
  Begin(); Rect(30.95,6.077,1.6,0.8); Extrude("Composant_Eagle_24",1.6,0.7,false);
  Begin(); Rect(29.426,6.077,1.6,0.8); Extrude("Composant_Eagle_25",1.6,0.7,false);
  Begin(); Rect(27.775,-1.323,1.6,2.9); Extrude("Composant_Eagle_26",1.6,0.7,false);
  Begin(); Rect(24.473,-1.323,1.6,2.9); Extrude("Composant_Eagle_27",1.6,0.7,false);
  Begin(); Rect(27.267,2.775,1.6,0.8); Extrude("Composant_Eagle_28",1.6,0.7,false);
  Begin(); Rect(23.457,2.775,1.6,0.8); Extrude("Composant_Eagle_29",1.6,0.7,false);
  Begin(); Rect(29.807,2.775,1.6,0.8); Extrude("Composant_Eagle_30",1.6,0.7,false);
  IPartDoc part=(IPartDoc)doc;
  object[] bodies=(object[])part.GetBodies2(0,false);
  if(bodies==null || bodies.Length<10) throw new Exception("Missing solids");
  int boardCount=0;
  foreach(IBody2 b in bodies) {
   double[] bb=(double[])b.GetBodyBox(); double dx=bb[3]-bb[0],dy=bb[4]-bb[1],dz=bb[5]-bb[2];
   double[] col;
   if(dx>0.079) {col=new double[]{0.02,0.35,0.46,1,0.7,0.2,0.2,0,0}; b.Name="PCB_Seeed_v1_0";boardCount++;}
   else if(dx>0.039 && dy>0.037 && dz>0.01) {col=new double[]{0.06,0.06,0.07,1,0.7,0.2,0.2,0,0};b.Name="Corps_HM3301";}
   else if(dx>0.035 || dy>0.035) {col=new double[]{0.0,0.65,0.75,1,0.7,0.4,0.3,0,0};}
   else if(dy>0.010 || (dx>0.008 && dz<0.0001)) col=new double[]{0.9,0.9,0.84,1,0.7,0.1,0.1,0,0};
   else col=new double[]{0.45,0.47,0.5,1,0.6,0.6,0.4,0,0};
   b.MaterialPropertyValues2=col;
  }
  if(boardCount!=1) throw new Exception("PCB count mismatch");
  doc.ForceRebuild3(false); doc.ShowNamedView2("*Isometric",7);doc.ViewZoomtofit2();
  int errors=0,warnings=0;
  string target=Path.Combine(folder,"Grove_HM3301.SLDPRT");
  if(!doc.Extension.SaveAs(target,0,1,null,ref errors,ref warnings)) throw new Exception("Save SLDPRT error "+errors);
  Console.WriteLine("SLDPRT: "+target+" warnings="+warnings);
  Console.WriteLine("SLDPRT solids="+bodies.Length);
  bool bmp=doc.SaveBMP(Path.Combine(folder,"apercu.bmp"),1400,800);
  Console.WriteLine("Preview="+bmp);
 }
}
'@
[HM3301Model]::Run($PSScriptRoot)

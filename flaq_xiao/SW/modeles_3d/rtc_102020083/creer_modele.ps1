$ErrorActionPreference = 'Stop'
$dll = 'C:\Program Files\SOLIDWORKS Corp\SOLIDWORKS\SolidWorks.Interop.sldworks.dll'
Add-Type -Path $dll
Add-Type -ReferencedAssemblies $dll -TypeDefinition @'
using System;
using System.IO;
using SolidWorks.Interop.sldworks;
public class RTCModel {
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
  Wire(-18.565,10.0,-19.365000000000002,9.2,90);
  Wire(-19.365000000000002,-9.2,-18.565,-10.0,90);
  Wire(19.835,-10.0,20.634999999999998,-9.2,90);
  Wire(20.634999999999998,-9.2,20.634999999999998,-2.1,0);
  Wire(20.634999999999998,2.1,20.634999999999998,9.2,0);
  Wire(20.634999999999998,9.2,19.835,10.0,90);
  Wire(19.835,10.0,13.334999999999999,10.0,0);
  Wire(13.334999999999999,10.0,13.035,9.7,90);
  Wire(-18.565,-10.0,-11.465,-10.0,0);
  Wire(-11.465,-10.0,-11.265,-10.2,-90);
  Wire(-7.265000000000001,-10.0,-7.465,-10.2,90);
  Wire(-7.465,-10.2,-11.265,-10.2,-167.981988);
  Wire(-18.565,10.0,-11.565000000000001,10.0,0);
  Wire(-11.565000000000001,10.0,-11.265,10.3,90);
  Wire(-7.165000000000001,10.0,7.9350000000000005,10.0,0);
  Wire(7.9350000000000005,10.0,8.235,9.7,-90);
  Wire(-7.165000000000001,10.0,-7.465,10.3,-90);
  Wire(-11.265,10.3,-7.465,10.3,-162.054747);
  Wire(8.235,9.7,13.035,9.7,165.749967);
  Wire(20.634999999999998,-2.1,20.835,-1.9,-90);
  Wire(20.634999999999998,2.1,20.835,1.9,90);
  Wire(20.835,1.9,20.835,-1.9,-167.981988);
  Wire(7.9350000000000005,-10.0,8.235,-9.7,90);
  Wire(13.334999999999999,-10.0,13.035,-9.7,-90);
  Wire(13.035,-9.7,8.235,-9.7,165.749967);
  Wire(7.9350000000000005,-10.0,-7.265000000000001,-10.0,0);
  Wire(13.334999999999999,-10.0,19.835,-10.0,0);
  Wire(-19.365000000000002,2.7,-19.065,2.4,90);
  Wire(-19.365000000000002,-2.7,-19.065,-2.4,-90);
  Wire(-19.065,-2.4,-19.065,2.4,165.749967);
  Wire(-19.365000000000002,-2.7,-19.365000000000002,-9.2,0);
  Wire(-19.365000000000002,2.7,-19.365000000000002,9.2,0);
  Circle(-9.365,-10.0,1.1);
  Circle(-9.365,10.0,1.1);
  Circle(20.634999999999998,-0.0,1.1);

  Circle(4.555,0,0.75);Circle(11.955,0,0.75);
  Circle(-3.175,7.62,0.4445);Circle(-5.715,7.62,0.4445);
  Extrude("PCB_contour_et_percages_Eagle_ep_ESTIMEE_1_6",0,1.6,false);
  // shortcut: connector height and housing details estimated; use supplier drawing for fabrication.
  Begin();Rect(-17.973,-6.1,5.1,12.2);Extrude("Connecteur_Grove_H_ESTIMEE",1.6,8.4,false);
  Begin();Rect(-17.273,-5.4,3.7,10.8);Cut("Cavite_Grove",10,7.3);
  Begin();Rect(-13.673,-4,1,1);Rect(-13.673,3,1,1);Cut("Detrompeurs_simplifies",10,3.2);
  Begin();for(int i=0;i<4;i++)Rect(-15.8,-3.25+2*i,0.5,0.5);Extrude("Broches_Grove_pas2",2.7,5.9,false);
  Begin();Circle(8.255,0,7.4);Circle(8.255,0,6.4);Extrude("Support_pile_contour_Eagle_H4_1",1.6,4.1,false);
  Begin();Rect(0.2,-3.5,4,7);Cut("Ouverture_support_ESTIMEE",5.7,3.3);
  Begin();Circle(8.255,0,6.4);Extrude("Fond_support_simplifie",1.6,0.5,false);
  Begin();Rect(0.355,-7.1,1,14.2);Extrude("Dos_support_pile_ESTIME",1.6,4.1,false);
  Begin();Rect(1.405,-6.3,0.2,12.6);Extrude("Contact_positif_support_simplifie",2.4,3.2,false);
  Begin();Rect(5.2,-2.5,7.7,5);Extrude("Contact_fond_support_simplifie",2.1,0.15,false);
  Begin();Rect(7.5,-0.8,4.5,1.6);Extrude("Lame_ressort_simplifiee",2.25,0.25,false);

  Begin();Rect(-7.315,-4.306,3.2,1.5);Extrude("X1_position_Eagle",1.6,0.55,false);
  Begin();Rect(-6.207,-1.5,2,3);Extrude("U2_position_Eagle",1.6,0.75,false);
  Begin();Rect(-8.882,-3.806,1,0.5);Extrude("C1_position_Eagle",1.6,0.5,false);
  Begin();Rect(-3.548,-3.806,1,0.5);Extrude("C3_position_Eagle",1.6,0.5,false);
  Begin();Rect(-5.834,3.56,1,0.5);Extrude("R1_position_Eagle",1.6,0.5,false);
  Begin();Rect(-3.548,-0.25,1,0.5);Extrude("R3_position_Eagle",1.6,0.5,false);
  Begin();Rect(-3.355,3.16,2.9,1.3);Extrude("U1_position_Eagle",1.6,1,false);
  Begin();Rect(-6.977,3.56,1,0.5);Extrude("C2_position_Eagle",1.6,0.5,false);
  Begin();Rect(-4.691,3.56,1,0.5);Extrude("R2_position_Eagle",1.6,0.5,false);
  Begin();Rect(-8.747,-0.625,2,1.25);Extrude("C4_position_Eagle",1.6,0.7,false);
  Begin();Rect(-4.572,-4.318,0.762,1.524);Extrude("Pad_X1_0",1.6,0.05,false);
  Begin();Rect(-7.62,-4.318,0.762,1.524);Extrude("Pad_X1_1",1.6,0.05,false);
  Begin();Rect(-13.95,-3.5,2.5,1);Extrude("Pad_J1_0",1.6,0.05,false);
  Begin();Rect(-13.95,-1.5,2.5,1);Extrude("Pad_J1_1",1.6,0.05,false);
  Begin();Rect(-13.95,0.5,2.5,1);Extrude("Pad_J1_2",1.6,0.05,false);
  Begin();Rect(-13.95,2.5,2.5,1);Extrude("Pad_J1_3",1.6,0.05,false);
  Begin();Rect(-17.073,-6.8,2.5,2);Extrude("Pad_J1_4",1.6,0.05,false);
  Begin();Rect(-17.073,4.8,2.5,2);Extrude("Pad_J1_5",1.6,0.05,false);
  Begin();Rect(-6.082,-1.85,0.25,0.7);Extrude("Pad_U2_0",1.6,0.05,false);
  Begin();Rect(-5.582,-1.85,0.25,0.7);Extrude("Pad_U2_1",1.6,0.05,false);
  Begin();Rect(-5.082,-1.85,0.25,0.7);Extrude("Pad_U2_2",1.6,0.05,false);
  Begin();Rect(-4.582,-1.85,0.25,0.7);Extrude("Pad_U2_3",1.6,0.05,false);
  Begin();Rect(-4.582,1.15,0.25,0.7);Extrude("Pad_U2_4",1.6,0.05,false);
  Begin();Rect(-5.082,1.15,0.25,0.7);Extrude("Pad_U2_5",1.6,0.05,false);
  Begin();Rect(-5.582,1.15,0.25,0.7);Extrude("Pad_U2_6",1.6,0.05,false);
  Begin();Rect(-6.082,1.15,0.25,0.7);Extrude("Pad_U2_7",1.6,0.05,false);
  Begin();Rect(-5.957,-0.75,1.5,1.5);Extrude("Pad_U2_8",1.6,0.05,false);
  Begin();Rect(-8.6995,-4.358,0.635,0.635);Extrude("Pad_C1_0",1.6,0.05,false);
  Begin();Rect(-8.6995,-3.389,0.635,0.635);Extrude("Pad_C1_1",1.6,0.05,false);
  Begin();Rect(-3.3655,-4.358,0.635,0.635);Extrude("Pad_C3_0",1.6,0.05,false);
  Begin();Rect(-3.3655,-3.389,0.635,0.635);Extrude("Pad_C3_1",1.6,0.05,false);
  Begin();Rect(-5.6515,3.008,0.635,0.635);Extrude("Pad_R1_0",1.6,0.05,false);
  Begin();Rect(-5.6515,3.977,0.635,0.635);Extrude("Pad_R1_1",1.6,0.05,false);
  Begin();Rect(-3.3655,0.167,0.635,0.635);Extrude("Pad_R3_0",1.6,0.05,false);
  Begin();Rect(-3.3655,-0.802,0.635,0.635);Extrude("Pad_R3_1",1.6,0.05,false);
  Begin();Rect(-2.413,4.2545,1.016,1.143);Extrude("Pad_U1_0",1.6,0.05,false);
  Begin();Rect(-1.524,2.2225,1.016,1.143);Extrude("Pad_U1_1",1.6,0.05,false);
  Begin();Rect(-3.302,2.2225,1.016,1.143);Extrude("Pad_U1_2",1.6,0.05,false);
  Begin();Rect(-6.7945,3.008,0.635,0.635);Extrude("Pad_C2_0",1.6,0.05,false);
  Begin();Rect(-6.7945,3.977,0.635,0.635);Extrude("Pad_C2_1",1.6,0.05,false);
  Begin();Rect(-4.5085,3.008,0.635,0.635);Extrude("Pad_R2_0",1.6,0.05,false);
  Begin();Rect(-4.5085,3.977,0.635,0.635);Extrude("Pad_R2_1",1.6,0.05,false);
  Begin();Rect(-2.145,-1,4.4,2);Extrude("Pad_BAT1_0",1.6,0.05,false);
  Begin();Rect(14.255,-1,4.4,2);Extrude("Pad_BAT1_1",1.6,0.05,false);
  Begin();Rect(-8.4455,-1.397,1.397,1.016);Extrude("Pad_C4_0",1.6,0.05,false);
  Begin();Rect(-8.4455,0.381,1.397,1.016);Extrude("Pad_C4_1",1.6,0.05,false);

  Begin();Circle(-3.175,7.62,.8255);Circle(-3.175,7.62,.4445);Circle(-5.715,7.62,.8255);Circle(-5.715,7.62,.4445);Extrude("Anneaux_CLK_INT",1.6,0.05,false);
  IPartDoc part=(IPartDoc)doc;object[] bodies=(object[])part.GetBodies2(0,false);
  int pcb=0,holder=0,connector=0;
  foreach(IBody2 b in bodies){double[] bb=(double[])b.GetBodyBox();double dx=bb[3]-bb[0],dy=bb[4]-bb[1],dz=bb[5]-bb[2];double[] c;
   if(dx>0.039){c=new double[]{0.02,0.35,0.49,1,0.7,0.2,0.2,0,0};b.Name="Carte_Grove_102020083";pcb++;}
   else if(dx>0.014 && dy>0.014){c=new double[]{0.20,0.18,0.20,1,0.7,0.1,0.1,0,0};b.Name="Support_pile_vide";holder++;}
   else if(dy>0.012 && dz>0.008){c=new double[]{0.92,0.91,0.86,1,0.7,0.1,0.1,0,0};b.Name="Connecteur_Grove";connector++;}
   else if(dz<0.0003 || dz>0.005 || (dx<.0003 && dy>.01)){c=new double[]{0.78,0.67,0.34,1,0.6,0.6,0.4,0,0};}
   else c=new double[]{0.20,0.18,0.20,1,0.7,0.1,0.1,0,0};
   b.MaterialPropertyValues2=c;
  }
  if(pcb!=1 || holder!=1 || connector!=1 || bodies.Length<30)throw new Exception("Geometry check failed");
  doc.ForceRebuild3(false);doc.ShowNamedView2("*Isometric",7);doc.ViewZoomtofit2();
  int err=0,warn=0;string target=Path.Combine(folder,"Grove_RTC_PCF85063TP_102020083.SLDPRT");
  if(!doc.Extension.SaveAs(target,0,1,null,ref err,ref warn))throw new Exception("Save failed "+err);
  Console.WriteLine("Saved "+target+"; bodies="+bodies.Length+"; warnings="+warn);
  doc.SaveBMP(Path.Combine(folder,"apercu.bmp"),1200,900);
 }
}
'@
[RTCModel]::Run($PSScriptRoot)

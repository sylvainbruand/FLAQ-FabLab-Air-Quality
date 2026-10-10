$ErrorActionPreference = 'Stop'
$dll = 'C:\Program Files\SOLIDWORKS Corp\SOLIDWORKS\SolidWorks.Interop.sldworks.dll'
Add-Type -Path $dll
Add-Type -ReferencedAssemblies $dll -TypeDefinition @'
using System;
using System.IO;
using SolidWorks.Interop.sldworks;
public class PassiveBuzzerModel {
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
  Wire(-9.1873,10.0127,-9.9873,9.2127,90);
  Wire(-9.9873,-9.1873,-9.1873,-9.9873,90);
  Wire(-9.1873,-9.9873,-2.0873,-9.9873,0);
  Wire(9.2127,-9.9873,10.0127,-9.1873,90);
  Wire(9.2127,10.0127,10.0127,9.2127,-90);
  Wire(-2.0873,-9.9873,-1.8873,-10.1873,-90);
  Wire(2.1127,-9.9873,1.9127,-10.1873,90);
  Wire(1.9127,-10.1873,-1.8873,-10.1873,-167.981988);
  Wire(-9.1873,10.0127,-2.1873,10.0127,0);
  Wire(-2.1873,10.0127,-1.8873,10.3127,90);
  Wire(2.2127,10.0127,1.9127,10.3127,-90);
  Wire(-1.8873,10.3127,1.9127,10.3127,-162.054747);
  Wire(-9.9873,2.7127,-9.6873,2.4127,90);
  Wire(-9.9873,-2.6873,-9.6873,-2.3873,-90);
  Wire(-9.6873,-2.3873,-9.6873,2.4127,165.749967);
  Wire(-9.9873,-2.6873,-9.9873,-9.1873,0);
  Wire(-9.9873,2.7127,-9.9873,9.2127,0);
  Wire(9.2127,-9.9873,2.1127,-9.9873,0);
  Wire(10.0127,-2.6873,9.7127,-2.3873,90);
  Wire(10.0127,2.7127,9.7127,2.4127,-90);
  Wire(9.7127,2.4127,9.7127,-2.3873,165.749967);
  Wire(10.0127,2.7127,10.0127,9.2127,0);
  Wire(10.0127,-2.6873,10.0127,-9.1873,0);
  Wire(2.2127,10.0127,9.2127,10.0127,0);
  Circle(0.0127,-9.9873,1.1);
  Circle(0.0127,10.0127,1.1);
  Extrude("PCB_contour_Eagle_ep_ESTIMEE_1_6",0,1.6,false);

  // shortcut: connector height and small housing details are estimates; replace with connector drawing if available.
  Begin();Rect(-9.083,-5.973,5.1,12.2);Extrude("Grove_position_Eagle_H_ESTIMEE",1.6,8.4,false);
  Begin();Rect(-8.383,-5.273,3.7,10.8);Cut("Cavite_connecteur",10,7.3);
  Begin();Rect(-4.783,-3.873,1,1);Rect(-4.783,3.127,1,1);Cut("Detrompeurs_ESTIMES",10,3.2);
  Begin();for(int i=0;i<4;i++) Rect(-6.9,-3.123+2*i,0.5,0.5);Extrude("Contacts_Grove_pas_2mm",2.7,5.9,false);
  Begin();
  double[,] p={{-2.75,-4.25},{2.75,-4.25},{4.25,-2.75},{4.25,2.75},{2.75,4.25},{-2.75,4.25},{-4.25,2.75},{-4.25,-2.75}};
  for(int i=0;i<8;i++) Wire(4.699+p[i,0],p[i,1],4.699+p[(i+1)%8,0],p[(i+1)%8,1],0);
  Extrude("MLT8530_8_5x8_5x3",1.6,3,false);
  Begin();Rect(4.199,3.75,1,0.8);Cut("Sortie_son_laterale_ESTIMEE",3.6,1.4);

  Begin();Rect(-0.527,5.804,0.8,1.6);Extrude("C1_position_Eagle_H_simplifiee",1.6,0.7,false);
  Begin();Rect(-2.432,5.804,0.8,1.6);Extrude("C2_position_Eagle_H_simplifiee",1.6,0.7,false);
  Begin();Rect(2.995,-7.254,2.9,1.3);Extrude("Q1_position_Eagle_H_simplifiee",1.6,0.7,false);
  Begin();Rect(-0.527,-7.404,0.8,1.6);Extrude("R1_position_Eagle_H_simplifiee",1.6,0.7,false);
  Begin();Rect(3.668,5.154,1.3,2.9);Extrude("U1_position_Eagle_H_simplifiee",1.6,0.7,false);
  Begin();Rect(1.378,5.804,0.8,1.6);Extrude("C3_position_Eagle_H_simplifiee",1.6,0.7,false);
  Begin();Rect(8.236,5.804,0.8,1.6);Extrude("C5_position_Eagle_H_simplifiee",1.6,0.7,false);
  Begin();Rect(6.585,5.804,0.8,1.6);Extrude("C4_position_Eagle_H_simplifiee",1.6,0.7,false);
  Begin();Rect(-2.432,-7.404,0.8,1.6);Extrude("R2_position_Eagle_H_simplifiee",1.6,0.7,false);
  Begin();Rect(-7.785,-8.655,1.6,0.8);Extrude("D1_position_Eagle_H_simplifiee",1.6,0.7,false);
  Begin();Rect(6.458,-7.531,0.8,1.6);Extrude("D2_position_Eagle_H_simplifiee",1.6,0.7,false);
  Begin();Rect(1.378,-7.404,0.8,1.6);Extrude("R3_position_Eagle_H_simplifiee",1.6,0.7,false);
  Begin();Rect(-0.5715,6.9215,0.889,0.889);Extrude("Pad_C1_0",1.6,0.05,false);
  Begin();Rect(-0.5715,5.3975,0.889,0.889);Extrude("Pad_C1_1",1.6,0.05,false);
  Begin();Rect(-2.4765,6.9215,0.889,0.889);Extrude("Pad_C2_0",1.6,0.05,false);
  Begin();Rect(-2.4765,5.3975,0.889,0.889);Extrude("Pad_C2_1",1.6,0.05,false);
  Begin();Rect(-5.06,-3.373,2.5,1);Extrude("Pad_J1_0",1.6,0.05,false);
  Begin();Rect(-5.06,-1.373,2.5,1);Extrude("Pad_J1_1",1.6,0.05,false);
  Begin();Rect(-5.06,0.627,2.5,1);Extrude("Pad_J1_2",1.6,0.05,false);
  Begin();Rect(-5.06,2.627,2.5,1);Extrude("Pad_J1_3",1.6,0.05,false);
  Begin();Rect(-8.183,-6.673,2.5,2);Extrude("Pad_J1_4",1.6,0.05,false);
  Begin();Rect(-8.183,4.927,2.5,2);Extrude("Pad_J1_5",1.6,0.05,false);
  Begin();Rect(3.937,-8.1915,1.016,1.143);Extrude("Pad_Q1_0",1.6,0.05,false);
  Begin();Rect(3.048,-6.1595,1.016,1.143);Extrude("Pad_Q1_1",1.6,0.05,false);
  Begin();Rect(4.826,-6.1595,1.016,1.143);Extrude("Pad_Q1_2",1.6,0.05,false);
  Begin();Rect(-0.5715,-7.8105,0.889,0.889);Extrude("Pad_R1_0",1.6,0.05,false);
  Begin();Rect(-0.5715,-6.2865,0.889,0.889);Extrude("Pad_R1_1",1.6,0.05,false);
  Begin();Rect(-0.051,-4.75,2,2);Extrude("Pad_BUZ1_0",1.6,0.05,false);
  Begin();Rect(7.449,-4.75,2,2);Extrude("Pad_BUZ1_1",1.6,0.05,false);
  Begin();Rect(7.449,2.75,2,2);Extrude("Pad_BUZ1_2",1.6,0.05,false);
  Begin();Rect(-0.051,2.75,2,2);Extrude("Pad_BUZ1_3",1.6,0.05,false);
  Begin();Rect(2.7305,6.096,1.143,1.016);Extrude("Pad_U1_0",1.6,0.05,false);
  Begin();Rect(4.7625,6.985,1.143,1.016);Extrude("Pad_U1_1",1.6,0.05,false);
  Begin();Rect(4.7625,5.207,1.143,1.016);Extrude("Pad_U1_2",1.6,0.05,false);
  Begin();Rect(1.3335,6.9215,0.889,0.889);Extrude("Pad_C3_0",1.6,0.05,false);
  Begin();Rect(1.3335,5.3975,0.889,0.889);Extrude("Pad_C3_1",1.6,0.05,false);
  Begin();Rect(8.1915,5.3975,0.889,0.889);Extrude("Pad_C5_0",1.6,0.05,false);
  Begin();Rect(8.1915,6.9215,0.889,0.889);Extrude("Pad_C5_1",1.6,0.05,false);
  Begin();Rect(6.5405,5.3975,0.889,0.889);Extrude("Pad_C4_0",1.6,0.05,false);
  Begin();Rect(6.5405,6.9215,0.889,0.889);Extrude("Pad_C4_1",1.6,0.05,false);
  Begin();Rect(-2.4765,-7.8105,0.889,0.889);Extrude("Pad_R2_0",1.6,0.05,false);
  Begin();Rect(-2.4765,-6.2865,0.889,0.889);Extrude("Pad_R2_1",1.6,0.05,false);
  Begin();Rect(-6.604,-8.6995,0.762,0.889);Extrude("Pad_D1_0",1.6,0.05,false);
  Begin();Rect(-8.128,-8.6995,0.762,0.889);Extrude("Pad_D1_1",1.6,0.05,false);
  Begin();Rect(6.508,-7.912,0.7,0.762);Extrude("Pad_D2_0",1.6,0.05,false);
  Begin();Rect(6.508,-6.312,0.7,0.762);Extrude("Pad_D2_1",1.6,0.05,false);
  Begin();Rect(1.3335,-7.8105,0.889,0.889);Extrude("Pad_R3_0",1.6,0.05,false);
  Begin();Rect(1.3335,-6.2865,0.889,0.889);Extrude("Pad_R3_1",1.6,0.05,false);

  IPartDoc part=(IPartDoc)doc;object[] bodies=(object[])part.GetBodies2(0,false);
  if(bodies==null || bodies.Length<30) throw new Exception("Missing solids");
  int pcb=0,buzzer=0,connector=0;
  foreach(IBody2 b2 in bodies){double[] bb=(double[])b2.GetBodyBox();double dx=bb[3]-bb[0],dy=bb[4]-bb[1],dz=bb[5]-bb[2];double[] c;
   if(dx>0.019 && dy>0.019){c=new double[]{0.02,0.36,0.49,1,0.7,0.15,0.2,0,0};b2.Name="PCB_Seeed_107020109";pcb++;}
   else if(dx>0.008 && dy>0.008 && dz>0.0029){c=new double[]{0.12,0.12,0.14,1,0.7,0.1,0.1,0,0};b2.Name="Buzzer_MLT8530";buzzer++;}
   else if(dy>0.012 && dz>0.008){c=new double[]{0.92,0.91,0.84,1,0.7,0.1,0.1,0,0};b2.Name="Connecteur_Grove";connector++;}
   else if(dz<0.0001 || dz>0.005){c=new double[]{0.70,0.72,0.74,1,0.6,0.6,0.4,0,0};}
   else {c=new double[]{0.20,0.20,0.21,1,0.7,0.2,0.1,0,0};}
   b2.MaterialPropertyValues2=c;
  }
  if(pcb!=1 || buzzer!=1 || connector!=1)throw new Exception("Geometry check failed");
  doc.ForceRebuild3(false);doc.ShowNamedView2("*Isometric",7);doc.ViewZoomtofit2();
  int errors=0,warnings=0;string target=Path.Combine(folder,"Grove_Buzzer_Passif_107020109.SLDPRT");
  if(!doc.Extension.SaveAs(target,0,1,null,ref errors,ref warnings))throw new Exception("Save error "+errors);
  Console.WriteLine("Saved "+target+"; solids="+bodies.Length+"; warnings="+warnings);
  doc.SaveBMP(Path.Combine(folder,"apercu.bmp"),1200,900);
 }
}
'@
[PassiveBuzzerModel]::Run($PSScriptRoot)

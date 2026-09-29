$ErrorActionPreference = 'Stop'
$dll = 'C:\Program Files\SOLIDWORKS Corp\SOLIDWORKS\SolidWorks.Interop.sldworks.dll'
Add-Type -Path $dll
Add-Type -ReferencedAssemblies $dll -TypeDefinition @'
using System;
using System.IO;
using SolidWorks.Interop.sldworks;
public class GroveModel {
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
  sw.CloseDoc("Grove_8_ports_TCA9548A_ESTIME.SLDPRT");
  doc=(IModelDoc2)sw.NewDocument(@"C:\ProgramData\SOLIDWORKS\SOLIDWORKS 2025\templates\Pièce.PRTDOT",0,0,0);
  if(doc==null) throw new Exception("NewDocument failed");
  for(IFeature f=(IFeature)doc.FirstFeature(); f!=null; f=(IFeature)f.GetNextFeature()) {
   if(f.GetTypeName2()=="RefPlane") { plane=f; break; }
  }
  if(plane==null) throw new Exception("Reference plane missing");
  sk=doc.SketchManager;
  // Dimensions inferred from product photo; not a manufacturer mechanical drawing.
  Begin(); Rect(0,0,80,20); Extrude("PCB_80x20_epaisseur_ESTIMEE_1_6",0,1.6,false);
  Begin(); foreach(double x in new double[]{10,50}) {Circle(x,0,2);Circle(x,20,2);} Circle(80,10,2);
  Extrude("Oreilles_fixation_ESTIMEES",0,1.6,true);
  Begin(); foreach(double x in new double[]{10,50}) {Circle(x,0,1.1);Circle(x,20,1.1);} Circle(80,10,1.1);
  foreach(double x in new double[]{30,70}) {Circle(x,0,2.6);Circle(x,20,2.6);} Circle(0,10,2.6);
  Cut("Percages_D2_2_et_encoches_ESTIMES",1.6,2);
  Begin(); foreach(double x in new double[]{35.6,38.14,40.68,43.22,54.8,57.34,59.88,62.42}) {Circle(x,1.6,0.5);Circle(x,18.4,0.5);} Circle(3.5,18.2,0.5);
  Cut("Trous_auxiliaires_ESTIMES",1.6,2);
  Begin(); for(int i=0;i<8;i++) Rect(21.5+7.3*i,4.7,6,10.6);
  Extrude("Huit_connecteurs_sorties_ESTIMES",1.6,6.4,false);
  Begin(); for(int i=0;i<8;i++) Rect(22.2+7.3*i,5.4,4.6,9.2);
  Cut("Cavites_connecteurs",8,5.4);
  Begin(); for(int i=0;i<8;i++) {Rect(26.7+7.3*i,6.5,1,1.0);Rect(26.7+7.3*i,12.5,1,1.0);}
  Cut("Encoches_detrompage_simplifiees",8,3.2);
  Begin(); Rect(0.2,4.7,8.5,10.6); Extrude("Connecteur_entree_coude_ESTIME",1.6,4.8,false);
  Begin(); Rect(-0.5,5.4,6.6,9.2); Cut("Ouverture_entree_simplifiee",5.7,3.1);
  Begin(); Rect(12,6.1,6,7.8); Extrude("TCA9548A_boitier_simplifie",1.6,1.2,false);
  Begin(); for(int i=0;i<8;i++) for(int p=0;p<4;p++) Rect(23.8+7.3*i,6.75+2*p,0.5,0.5);
  Extrude("Contacts_sorties_pas_2mm",2.6,4.2,false);
  Begin(); for(int p=0;p<4;p++) Rect(1.5,6.75+2*p,5,0.5);
  Extrude("Contacts_entree",3.0,0.5,false);

  IPartDoc part=(IPartDoc)doc;
  object[] bodies=(object[])part.GetBodies2(0,false);
  int boardCount=0,connectorCount=0;
  foreach(IBody2 b in bodies) {
   double[] bb=(double[])b.GetBodyBox(); double dx=bb[3]-bb[0],dy=bb[4]-bb[1],dz=bb[5]-bb[2];
   double[] col;
   if(dx>0.07) {col=new double[]{0.02,0.32,0.46,1,0.7,0.2,0.2,0,0};b.Name="PCB_estime";boardCount++;}
   else if(dz>0.004 && dx>0.005 && dy>0.009) {col=new double[]{0.92,0.92,0.88,1,0.7,0.15,0.1,0,0};connectorCount++;b.Name="Grove_"+connectorCount;}
   else if(dx>0.005 && dy>0.007) {col=new double[]{0.09,0.09,0.09,1,0.6,0.2,0.2,0,0};b.Name="TCA9548A";}
   else col=new double[]{0.7,0.72,0.74,1,0.5,0.7,0.5,0,0};
   b.MaterialPropertyValues2=col;
  }
  if(boardCount!=1 || connectorCount!=9) throw new Exception("Unexpected geometry: boards="+boardCount+" connectors="+connectorCount);
  doc.ForceRebuild3(false); doc.ShowNamedView2("*Isometric",7);doc.ViewZoomtofit2();
  int errors=0,warnings=0;
  string target=Path.Combine(folder,"Grove_8_ports_TCA9548A_ESTIME.SLDPRT");
  if(!doc.Extension.SaveAs(target,0,1,null,ref errors,ref warnings)) throw new Exception("Save SLDPRT error "+errors);
  Console.WriteLine("SLDPRT: "+target+" warnings="+warnings);
  Console.WriteLine("SLDPRT solids="+bodies.Length+" connectors="+connectorCount);
  bool bmp=doc.SaveBMP(Path.Combine(folder,"apercu.bmp"),1400,800);
  Console.WriteLine("Preview="+bmp);
 }
}
'@
[GroveModel]::Run($PSScriptRoot)

$ErrorActionPreference = 'Stop'
$dll = 'C:\Program Files\SOLIDWORKS Corp\SOLIDWORKS\SolidWorks.Interop.sldworks.dll'
Add-Type -Path $dll
Add-Type -ReferencedAssemblies $dll -TypeDefinition @'
using System;
using System.IO;
using SolidWorks.Interop.sldworks;
public class HCHOModel {
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
 Wire(-2.2,-10,-2.1,-10.1,-90);
 Wire(-2.1,-10.1,-0.1,-12.1,90);
 Wire(-0.1,-12.1,0.1,-12.1,0);
 Wire(0.1,-12.1,2.1,-10.1,90);
 Wire(2.1,-10.1,2.2,-10,-90);
 Wire(2.2,-10,9.2,-10,0);
 Wire(9.2,-10,10,-9.2,90);
 Wire(10,-9.2,10,-2.2,0);
 Wire(10,-2.2,9.9,-2.1,90);
 Wire(9.9,-2.1,7.9,-0.1,-90);
 Wire(7.9,-0.1,7.9,0.1,0);
 Wire(7.9,0.1,9.9,2.1,-90);
 Wire(9.9,2.1,10,2.2,90);
 Wire(10,2.2,10,9.2,0);
 Wire(10,9.2,9.2,10,90);
 Wire(9.2,10,2.2,10,0);
 Wire(2.2,10,2.1,10.1,-90);
 Wire(2.1,10.1,0.1,12.1,90);
 Wire(0.1,12.1,-0.1,12.1,0);
 Wire(-0.1,12.1,-2.1,10.1,90);
 Wire(-2.1,10.1,-2.2,10,-90);
 Wire(-2.2,10,-9.2,10,0);
 Wire(-9.2,10,-10,9.2,90);
 Wire(-10,9.2,-10,2.2,0);
 Wire(-10,2.2,-9.9,2.1,90);
 Wire(-9.9,2.1,-7.9,0.1,-90);
 Wire(-7.9,0.1,-7.9,-0.1,0);
 Wire(-7.9,-0.1,-9.9,-2.1,-90);
 Wire(-9.9,-2.1,-10,-2.2,90);
 Wire(-2.2,-10,-9.2,-10,0);
 Wire(-9.2,-10,-10,-9.2,-90);
 Wire(-10,-9.2,-10,-2.2,0);
 Circle(-0.0,10.0,1.1);
 Circle(-0.0,-10.0,1.1);
 Circle(2.895,-5.995,0.45);
 Circle(2.895,-2.895,0.45);
 Circle(5.995,-5.995,0.45);
 Circle(5.943,-2.895,0.45);
 Circle(-5.08,3,0.4);
 Circle(-5.08,1,0.4);
 Circle(-5.08,-1,0.4);
 Circle(-5.08,-3,0.4);
 Circle(8.255,5.965,0.4);
 Circle(3.175,5.965,0.4);
 Circle(5.715,3.465,0.4);

 Extrude("PCB_contour_et_percages_Eagle_ep_ESTIMEE1_6",0,1.6,false);
 // shortcut: connector, potentiometer heights and sensor mounting gap estimated; verify real part for close fitting.
 Begin();Rect(-7.88,-5,5,10);Extrude("Grove_vertical_H_ESTIMEE8_4",1.6,8.4,false);
 Begin();Rect(-7.18,-4.3,3.6,8.6);Cut("Cavite_Grove",10,7.3);
 Begin();Rect(-7.98,-3.2,1,1.4);Rect(-7.98,1.8,1,1.4);Cut("Detrompeurs_Grove",10,3.2);
 Begin();for(int i=0;i<4;i++)Rect(-5.33,-3.25+2*i,.5,.5);Extrude("Contacts_Grove",-.8,9.4,false);
 Begin();Circle(4.445,-4.445,4.7);Extrude("WSP2110_embase_diam9_4",1.8,.5,false);
 Begin();Circle(4.445,-4.445,4);Circle(4.445,-4.445,3.65);Extrude("WSP2110_corps_diam8_H7",2.3,6.5,false);
 Begin();Circle(4.445,-4.445,3.65);Circle(4.445,-4.445,2.6);Extrude("WSP2110_couronne_superieure",8.45,.35,false);
 Begin();Circle(4.445,-4.445,2.6);
 for(int i=-6;i<=6;i++)for(int j=-6;j<=6;j++)if(i*i+j*j<36)Circle(4.445+i*.38,-4.445+j*.38,.13);
 Extrude("WSP2110_grille_simplifiee",8.4,.15,false);
 Begin();Rect(2.315,2.215,6.8,7);Extrude("Potentiometre_R1_H_ESTIMEE4_5",1.6,4.5,false);
 Begin();Circle(5.715,5.715,1.5);Cut("Logement_reglage_R1",6.1,1);
 Begin();Circle(5.715,5.715,1.4);Extrude("Vis_reglage_R1",5.1,.8,false);
 Begin();Rect(4.215,5.415,3,.6);Cut("Fente_vis_R1",5.9,.4);
 Begin();Rect(5.415,4.215,.6,3);Cut("Croix_vis_R1",5.9,.4);
 Begin();Rect(-1.435,4.68,1.6,.8);Extrude("R2_position_Eagle",1.65,.55,false);
 Begin();Rect(-1.435,.87,1.6,.8);Extrude("C1_position_Eagle",1.65,.55,false);

 Begin();Circle(2.895,-5.995,0.8);Circle(2.895,-5.995,0.45);Extrude("Anneau_U1_0",1.6,.05,false);
 Begin();Circle(2.895,-5.995,.3);Extrude("Patte_U1_0",-1,2.8,false);
 Begin();Circle(2.895,-2.895,0.8);Circle(2.895,-2.895,0.45);Extrude("Anneau_U1_1",1.6,.05,false);
 Begin();Circle(2.895,-2.895,.3);Extrude("Patte_U1_1",-1,2.8,false);
 Begin();Circle(5.995,-5.995,0.8);Circle(5.995,-5.995,0.45);Extrude("Anneau_U1_2",1.6,.05,false);
 Begin();Circle(5.995,-5.995,.3);Extrude("Patte_U1_2",-1,2.8,false);
 Begin();Circle(5.943,-2.895,0.8);Circle(5.943,-2.895,0.45);Extrude("Anneau_U1_3",1.6,.05,false);
 Begin();Circle(5.943,-2.895,.3);Extrude("Patte_U1_3",-1,2.8,false);
 Begin();Circle(-5.08,3,0.635);Circle(-5.08,3,0.4);Extrude("Anneau_J1_0",1.6,.05,false);
 Begin();Circle(-5.08,1,0.635);Circle(-5.08,1,0.4);Extrude("Anneau_J1_1",1.6,.05,false);
 Begin();Circle(-5.08,-1,0.635);Circle(-5.08,-1,0.4);Extrude("Anneau_J1_2",1.6,.05,false);
 Begin();Circle(-5.08,-3,0.635);Circle(-5.08,-3,0.4);Extrude("Anneau_J1_3",1.6,.05,false);
 Begin();Circle(8.255,5.965,0.675);Circle(8.255,5.965,0.4);Extrude("Anneau_R1_0",1.6,.05,false);
 Begin();Circle(8.255,5.965,.3);Extrude("Patte_R1_0",-1,2.8,false);
 Begin();Circle(3.175,5.965,0.675);Circle(3.175,5.965,0.4);Extrude("Anneau_R1_1",1.6,.05,false);
 Begin();Circle(3.175,5.965,.3);Extrude("Patte_R1_1",-1,2.8,false);
 Begin();Circle(5.715,3.465,0.675);Circle(5.715,3.465,0.4);Extrude("Anneau_R1_2",1.6,.05,false);
 Begin();Circle(5.715,3.465,.3);Extrude("Patte_R1_2",-1,2.8,false);
 Begin();Rect(-1.0795,5.3975,0.889,0.889);Extrude("Pad_R2_0",1.6,.05,false);
 Begin();Rect(-1.0795,3.8735,0.889,0.889);Extrude("Pad_R2_1",1.6,.05,false);
 Begin();Rect(-1.0795,1.5875,0.889,0.889);Extrude("Pad_C1_0",1.6,.05,false);
 Begin();Rect(-1.0795,0.0635,0.889,0.889);Extrude("Pad_C1_1",1.6,.05,false);

 object[] bodies=(object[])((IPartDoc)doc).GetBodies2(0,false);int pcb=0,pot=0,connector=0;
 foreach(IBody2 b in bodies){double[] bb=(double[])b.GetBodyBox();double dx=bb[3]-bb[0],dy=bb[4]-bb[1],dz=bb[5]-bb[2];double[] c;
  if(dx>.019){c=new double[]{.02,.35,.49,1,.7,.2,.2,0,0};b.Name="Carte_Grove_HCHO_101020001";pcb++;}
  else if(dx>.006 && dy>.006 && bb[1]>.002 && dz>.004){c=new double[]{.06,.18,.72,1,.7,.2,.2,0,0};b.Name="Potentiometre_R1";pot++;}
  else if(dx>.004 && dy>.009 && dz>.008){c=new double[]{.92,.91,.86,1,.7,.1,.1,0,0};b.Name="Connecteur_Grove";connector++;}
  else if(dz<.0002 || dz>.002 || bb[0]>.001){c=new double[]{.75,.73,.61,1,.6,.6,.4,0,0};}
  else c=new double[]{.20,.18,.17,1,.7,.1,.1,0,0};
  b.MaterialPropertyValues2=c;
 }
 if(pcb!=1 || pot!=1 || connector!=1 || bodies.Length<30)throw new Exception("Geometry check failed: "+bodies.Length);
 doc.ForceRebuild3(false);doc.ShowNamedView2("*Isometric",7);doc.ViewZoomtofit2();
 int err=0,warn=0;string target=Path.Combine(folder,"Grove_HCHO_101020001.SLDPRT");
 if(!doc.Extension.SaveAs(target,0,1,null,ref err,ref warn))throw new Exception("Save failed "+err);
 Console.WriteLine("Saved "+target+"; bodies="+bodies.Length+"; warnings="+warn);
 doc.SaveBMP(Path.Combine(folder,"apercu.bmp"),1200,900);
 }
}
'@
[HCHOModel]::Run($PSScriptRoot)

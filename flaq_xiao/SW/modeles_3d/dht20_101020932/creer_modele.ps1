$ErrorActionPreference = 'Stop'
$dll = 'C:\Program Files\SOLIDWORKS Corp\SOLIDWORKS\SolidWorks.Interop.sldworks.dll'
Add-Type -Path $dll
Add-Type -ReferencedAssemblies $dll -TypeDefinition @'
using System;
using System.IO;
using SolidWorks.Interop.sldworks;
public class DHT20Model {
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
 Wire(-12.2,10,-19.2,10,0);
 Wire(-19.2,10,-20,9.2,90);
 Wire(-20,9.2,-20,2.2,0);
 Wire(-20,2.2,-19.9,2.1,90);
 Wire(-19.9,2.1,-17.9,0.1,-90);
 Wire(-17.9,0.1,-17.9,-0.1,0);
 Wire(-17.9,-0.1,-19.9,-2.1,-90);
 Wire(-19.9,-2.1,-20,-2.2,90);
 Wire(-20,-2.2,-20,-9.2,0);
 Wire(-20,-9.2,-19.2,-10,90);
 Wire(-19.2,-10,-12.2,-10,0);
 Wire(-12.2,-10,-12.1,-10.1,-90);
 Wire(-12.1,-10.1,-10.1,-12.1,90);
 Wire(-10.1,-12.1,-9.9,-12.1,0);
 Wire(-9.9,-12.1,-7.9,-10.1,90);
 Wire(-7.9,-10.1,-7.8,-10,-90);
 Wire(-7.8,-10,7.8,-10,0);
 Wire(7.8,-10,7.9,-9.9,90);
 Wire(7.9,-9.9,9.9,-7.9,-90);
 Wire(9.9,-7.9,10.1,-7.9,0);
 Wire(10.1,-7.9,12.1,-9.9,-90);
 Wire(12.1,-9.9,12.2,-10,90);
 Wire(12.2,-10,19.2,-10,0);
 Wire(19.2,-10,20,-9.2,90);
 Wire(20,-9.2,20,-2.2,0);
 Wire(20,-2.2,20.1,-2.1,-90);
 Wire(20.1,-2.1,22.1,-0.1,90);
 Wire(22.1,-0.1,22.1,0.1,0);
 Wire(22.1,0.1,20.1,2.1,90);
 Wire(20.1,2.1,20,2.2,-90);
 Wire(20,2.2,20,9.2,0);
 Wire(20,9.2,19.2,10,90);
 Wire(19.2,10,12.2,10,0);
 Wire(12.2,10,12.1,9.9,90);
 Wire(12.1,9.9,10.1,7.9,-90);
 Wire(10.1,7.9,9.9,7.9,0);
 Wire(9.9,7.9,7.9,9.9,-90);
 Wire(7.9,9.9,7.8,10,90);
 Wire(7.8,10,-7.8,10,0);
 Wire(-7.8,10,-7.9,10.1,-90);
 Wire(-7.9,10.1,-9.9,12.1,90);
 Wire(-9.9,12.1,-10.1,12.1,0);
 Wire(-10.1,12.1,-12.1,10.1,90);
 Wire(-12.1,10.1,-12.2,10,-90);
 Circle(-10,-10,1.1);
 Circle(-10,10,1.1);
 Circle(20,0,1.1);
 Circle(-16.51,-3,0.4);
 Circle(-16.51,-1,0.4);
 Circle(-16.51,1,0.4);
 Circle(-16.51,3,0.4);
 Circle(-4.064,3.81,0.4445);
 Circle(-4.064,1.27,0.4445);
 Circle(-4.064,-1.27,0.4445);
 Circle(-4.064,-3.81,0.4445);

 Extrude("PCB_Eagle_V2_1_ep_ESTIMEE_1_6",0,1.6,false);
 // shortcut: connector heights and ventilation details estimated; replace from measured part for close-fit fabrication.
 Begin();Rect(-18.71,-5,5,10);Extrude("Grove_vertical_H_ESTIMEE",1.6,8.4,false);
 Begin();Rect(-18.01,-4.3,3.6,8.6);Cut("Cavite_Grove",10,7.3);
 Begin();Rect(-14.51,-3.2,1,1.4);Rect(-14.51,1.8,1,1.4);Cut("Detrompeurs_Grove",10,3.2);
 Begin();for(int i=0;i<4;i++)Rect(-16.76,-3.25+2*i,.5,.5);Extrude("Contacts_Grove",-.8,9.4,false);
 Begin();Rect(-2.335,-6.3,16.1,12.6);Extrude("DHT20_enveloppe_16_1x12_6x5_8",1.8,5,false);
 Begin();Rect(-1.635,-5.6,14.7,11.2);Cut("DHT20_interieur_simplifie",6.8,4.4);
 Begin();Rect(-2.335,-6.3,16.1,12.6);
 for(int i=0;i<6;i++)for(int j=0;j<3;j++)Rect(-.8+2.54*i,-3.3+2.54*j,1.2,1.2);
 Extrude("DHT20_grille_aeration_simplifiee",6.8,.8,false);
 Begin();for(int i=0;i<3;i++){Rect(-.8+5.08*i,-6.4,1.2,1.6);Rect(-.8+5.08*i,4.8,1.2,1.6);}Cut("DHT20_encoches_laterales",7.6,2.6);
 Begin();for(int i=0;i<4;i++)Rect(-4.314,-4.06+2.54*i,1.979,.5);Extrude("Pattes_DHT20_horizontales",4.05,.3,false);
 Begin();for(int i=0;i<4;i++)Rect(-4.314,-4.06+2.54*i,.5,.5);Extrude("Pattes_DHT20_traversantes",-.8,4.85,false);

 Begin();Rect(-10.2258,3.0204,1.3,2.9);Extrude("U14_position_Eagle",1.65,1,false);
 Begin();Rect(-10.0504,1.909,1,0.5);Extrude("C62_position_Eagle",1.65,0.4,false);
 Begin();Rect(-7.231,5.4142,1,0.5);Extrude("C65_position_Eagle",1.65,0.4,false);
 Begin();Rect(-8.143,-1.7802,1.3,2.9);Extrude("Q12_position_Eagle",1.65,1,false);
 Begin();Rect(-8.143,-5.0314,1.3,2.9);Extrude("Q13_position_Eagle",1.65,1,false);
 Begin();Rect(-10.2576,-4.691,0.5,1);Extrude("R65_position_Eagle",1.65,0.4,false);
 Begin();Rect(-7.2818,1.8836,1,0.5);Extrude("R66_position_Eagle",1.65,0.4,false);
 Begin();Rect(-10.2576,-2.6336,0.5,1);Extrude("R67_position_Eagle",1.65,0.4,false);
 Begin();Rect(-10.2576,-0.5254,0.5,1);Extrude("R68_position_Eagle",1.65,0.4,false);
 Begin();Rect(-7.2564,4.2204,1,0.5);Extrude("R53_position_Eagle",1.65,0.4,false);
 Begin();Rect(-7.2564,3.052,1,0.5);Extrude("C54_position_Eagle",1.65,0.4,false);
 Begin();Circle(-16.51,-3,0.635);Circle(-16.51,-3,0.4);Extrude("Anneau_J8_0",1.6,.05,false);
 Begin();Circle(-16.51,-1,0.635);Circle(-16.51,-1,0.4);Extrude("Anneau_J8_1",1.6,.05,false);
 Begin();Circle(-16.51,1,0.635);Circle(-16.51,1,0.4);Extrude("Anneau_J8_2",1.6,.05,false);
 Begin();Circle(-16.51,3,0.635);Circle(-16.51,3,0.4);Extrude("Anneau_J8_3",1.6,.05,false);
 Begin();Circle(-4.064,3.81,0.762);Circle(-4.064,3.81,0.4445);Extrude("Anneau_U11_0",1.6,.05,false);
 Begin();Circle(-4.064,1.27,0.762);Circle(-4.064,1.27,0.4445);Extrude("Anneau_U11_1",1.6,.05,false);
 Begin();Circle(-4.064,-1.27,0.762);Circle(-4.064,-1.27,0.4445);Extrude("Anneau_U11_2",1.6,.05,false);
 Begin();Circle(-4.064,-3.81,0.762);Circle(-4.064,-3.81,0.4445);Extrude("Anneau_U11_3",1.6,.05,false);
 Begin();Rect(-11.1633,3.9624,1.143,1.016);Extrude("Pad_U14_0",1.6,.05,false);
 Begin();Rect(-9.1313,4.8514,1.143,1.016);Extrude("Pad_U14_1",1.6,.05,false);
 Begin();Rect(-9.1313,3.0734,1.143,1.016);Extrude("Pad_U14_2",1.6,.05,false);
 Begin();Rect(-10.3124,1.8415,0.635,0.635);Extrude("Pad_C62_0",1.6,.05,false);
 Begin();Rect(-9.4234,1.8415,0.635,0.635);Extrude("Pad_C62_1",1.6,.05,false);
 Begin();Rect(-7.493,5.3467,0.635,0.635);Extrude("Pad_C65_0",1.6,.05,false);
 Begin();Rect(-6.604,5.3467,0.635,0.635);Extrude("Pad_C65_1",1.6,.05,false);
 Begin();Rect(-9.0805,-0.8382,1.143,1.016);Extrude("Pad_Q12_0",1.6,.05,false);
 Begin();Rect(-7.0485,0.0508,1.143,1.016);Extrude("Pad_Q12_1",1.6,.05,false);
 Begin();Rect(-7.0485,-1.7272,1.143,1.016);Extrude("Pad_Q12_2",1.6,.05,false);
 Begin();Rect(-9.0805,-4.0894,1.143,1.016);Extrude("Pad_Q13_0",1.6,.05,false);
 Begin();Rect(-7.0485,-3.2004,1.143,1.016);Extrude("Pad_Q13_1",1.6,.05,false);
 Begin();Rect(-7.0485,-4.9784,1.143,1.016);Extrude("Pad_Q13_2",1.6,.05,false);
 Begin();Rect(-10.3251,-4.953,0.635,0.635);Extrude("Pad_R65_0",1.6,.05,false);
 Begin();Rect(-10.3251,-4.064,0.635,0.635);Extrude("Pad_R65_1",1.6,.05,false);
 Begin();Rect(-7.5438,1.8161,0.635,0.635);Extrude("Pad_R66_0",1.6,.05,false);
 Begin();Rect(-6.6548,1.8161,0.635,0.635);Extrude("Pad_R66_1",1.6,.05,false);
 Begin();Rect(-10.3251,-2.0066,0.635,0.635);Extrude("Pad_R67_0",1.6,.05,false);
 Begin();Rect(-10.3251,-2.8956,0.635,0.635);Extrude("Pad_R67_1",1.6,.05,false);
 Begin();Rect(-10.3251,0.1016,0.635,0.635);Extrude("Pad_R68_0",1.6,.05,false);
 Begin();Rect(-10.3251,-0.7874,0.635,0.635);Extrude("Pad_R68_1",1.6,.05,false);
 Begin();Rect(-6.6294,4.1529,0.635,0.635);Extrude("Pad_R53_0",1.6,.05,false);
 Begin();Rect(-7.5184,4.1529,0.635,0.635);Extrude("Pad_R53_1",1.6,.05,false);
 Begin();Rect(-7.5184,2.9845,0.635,0.635);Extrude("Pad_C54_0",1.6,.05,false);
 Begin();Rect(-6.6294,2.9845,0.635,0.635);Extrude("Pad_C54_1",1.6,.05,false);

 object[] bodies=(object[])((IPartDoc)doc).GetBodies2(0,false);int pcb=0,sensor=0,connector=0;
 foreach(IBody2 b in bodies){double[] bb=(double[])b.GetBodyBox();double dx=bb[3]-bb[0],dy=bb[4]-bb[1],dz=bb[5]-bb[2];double[] c;
  if(dx>.039){c=new double[]{.02,.35,.49,1,.7,.2,.2,0,0};b.Name="PCB_Grove_101020932";pcb++;}
  else if(dx>.015 && dy>.012){c=new double[]{.12,.11,.12,1,.7,.1,.1,0,0};sensor++;}
  else if(dx>.004 && dy>.009 && dz>.008){c=new double[]{.92,.91,.86,1,.7,.1,.1,0,0};connector++;}
  else if(dz<.0001 || dz>.003 || (bb[0]>-.005 && bb[0]<-.002)){c=new double[]{.75,.73,.67,1,.6,.6,.4,0,0};}
  else c=new double[]{.17,.15,.15,1,.7,.1,.1,0,0};
  b.MaterialPropertyValues2=c;
 }
 if(pcb!=1 || sensor!=2 || connector!=1 || bodies.Length<50)throw new Exception("Geometry check: "+pcb+","+sensor+","+connector+","+bodies.Length);
 doc.ForceRebuild3(false);doc.ShowNamedView2("*Isometric",7);doc.ViewZoomtofit2();
 int err=0,warn=0;string target=Path.Combine(folder,"Grove_DHT20_101020932.SLDPRT");
 if(!doc.Extension.SaveAs(target,0,1,null,ref err,ref warn))throw new Exception("Save failed "+err);
 Console.WriteLine("Saved "+target+"; bodies="+bodies.Length+"; warnings="+warn);
 doc.SaveBMP(Path.Combine(folder,"apercu.bmp"),1200,900);
 }
}
'@
[DHT20Model]::Run($PSScriptRoot)

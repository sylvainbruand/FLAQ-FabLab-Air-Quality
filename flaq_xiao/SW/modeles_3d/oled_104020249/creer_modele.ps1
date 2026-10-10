$ErrorActionPreference = 'Stop'
$dll = 'C:\Program Files\SOLIDWORKS Corp\SOLIDWORKS\SolidWorks.Interop.sldworks.dll'
Add-Type -Path $dll
Add-Type -ReferencedAssemblies $dll -TypeDefinition @'
using System;
using System.IO;
using SolidWorks.Interop.sldworks;
public class OLED249Model {
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
 Wire(8.12,20.16,1.12,20.16,0);
 Wire(1.12,20.16,0.32,19.36,90);
 Wire(0.32,19.36,0.32,12.36,0);
 Wire(0.32,12.36,0.42,12.26,90);
 Wire(0.42,12.26,2.42,10.26,-90);
 Wire(2.42,10.26,2.42,10.06,0);
 Wire(2.42,10.06,0.42,8.06,-90);
 Wire(0.42,8.06,0.32,7.96,90);
 Wire(0.32,7.96,0.32,0.96,0);
 Wire(0.32,0.96,1.12,0.16,90);
 Wire(1.12,0.16,8.12,0.16,0);
 Wire(8.12,0.16,8.22,0.06,-90);
 Wire(8.22,0.06,10.22,-1.94,90);
 Wire(10.22,-1.94,10.42,-1.94,0);
 Wire(10.42,-1.94,12.42,0.06,90);
 Wire(12.42,0.06,12.52,0.16,-90);
 Wire(12.52,0.16,28.12,0.16,0);
 Wire(28.12,0.16,28.22,0.26,90);
 Wire(28.22,0.26,30.22,2.26,-90);
 Wire(30.22,2.26,30.42,2.26,0);
 Wire(30.42,2.26,32.42,0.26,-90);
 Wire(32.42,0.26,32.52,0.16,90);
 Wire(32.52,0.16,39.52,0.16,0);
 Wire(39.52,0.16,40.32,0.96,90);
 Wire(40.32,0.96,40.32,7.96,0);
 Wire(40.32,7.96,40.42,8.06,-90);
 Wire(40.42,8.06,42.42,10.06,90);
 Wire(42.42,10.06,42.42,10.26,0);
 Wire(42.42,10.26,40.42,12.26,90);
 Wire(40.42,12.26,40.32,12.36,-90);
 Wire(40.32,12.36,40.32,19.36,0);
 Wire(40.32,19.36,39.52,20.16,90);
 Wire(39.52,20.16,32.52,20.16,0);
 Wire(32.52,20.16,32.42,20.06,90);
 Wire(32.42,20.06,30.42,18.06,-90);
 Wire(30.42,18.06,30.22,18.06,0);
 Wire(30.22,18.06,28.22,20.06,-90);
 Wire(28.22,20.06,28.12,20.16,90);
 Wire(28.12,20.16,12.52,20.16,0);
 Wire(12.52,20.16,12.42,20.26,-90);
 Wire(12.42,20.26,10.42,22.26,90);
 Wire(10.42,22.26,10.22,22.26,0);
 Wire(10.22,22.26,8.22,20.26,90);
 Wire(8.22,20.26,8.12,20.16,-90);
 Circle(10.32,0.16,1.1);
 Circle(10.32,20.16,1.1);
 Circle(40.32,10.16,1.1);
 Circle(37.0078,15.2908,0.5461);
 Circle(37.0078,12.7508,0.5461);
 Circle(37.0078,17.8308,0.5461);
 Circle(37.0078,10.2108,0.5461);
 Circle(37.0078,7.6708,0.5461);
 Circle(37.0078,5.1308,0.5461);
 Circle(37.0078,2.5908,0.5461);

 Extrude("PCB_104020249_source_V1_0_ep_ESTIMEE1_6",0,1.6,false);
 // shortcut: published PCB is v1.0; v1.1 compatibility and mounting heights require physical verification.
 Begin();Rect(4.37,3.71,21.74,13);Extrude("Support_dalle_H_ESTIMEE1",1.6,1,false);
 Begin();Rect(2.87,1.7608,24.74,16.9);Extrude("Dalle24_74x16_9x1_42",2.6,1.4,false);
 Begin();Rect(2.87,1.7608,24.74,16.9);Rect(4.37,6.3008,21.74,10.86);Extrude("Bordure_dalle",4,.02,false);
 Begin();Rect(4.37,6.3008,21.74,10.86);Extrude("Zone_active_ecran_eteint",4,.02,false);
 Begin();Rect(11.24,1.41,8,.35);Extrude("Nappe_repliee_simplifiee",1.8,2.2,false);
 Begin();Rect(.95,4.06,5.1,12.2);Rect(1.65,4.76,3.7,10.8);Extrude("Grove_vertical_dessous_H_ESTIMEE8_4",-8.4,7.3,false);
 Begin();Rect(.95,4.06,5.1,12.2);Extrude("Grove_fond",-1.1,1.1,false);
 Begin();for(int i=0;i<4;i++)Rect(3.25,6.91+2*i,.5,.5);Extrude("Contacts_Grove_dessous",-7,.9+5,false);

 Begin();Rect(24.7882,2.8298,2.9,1.3);Extrude("U1_dessous_Eagle",-1.05,1,false);
 Begin();Rect(25.42320625,6.7857,1.6,0.8);Extrude("C2_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(12.5794,5.55,0.8,1.6);Extrude("C4_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(26.94915,6.7857,1.6,0.8);Extrude("C1_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(12.5794,4.17416563,0.8,1.6);Extrude("C5_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(7.22,16.8276,0.8,1.6);Extrude("C6_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(9.76,16.8276,0.8,1.6);Extrude("C7_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(17.38,16.8276,0.8,1.6);Extrude("C8_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(19.92,16.8276,0.8,1.6);Extrude("C9_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(17.38,14.9226,0.8,1.6);Extrude("R11_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(12.3,16.8276,0.8,1.6);Extrude("R10_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(15.8077,5.55,0.8,1.6);Extrude("C3_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(14.767,7.3216,1.2,0.8);Extrude("D1_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(12.5794,2.79833125,0.8,1.6);Extrude("R4_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(15.8077,2.79833125,0.8,1.6);Extrude("R5_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(12.3,14.9226,0.8,1.6);Extrude("R6_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(7.22,14.9226,0.8,1.6);Extrude("R7_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(14.84,16.8276,0.8,1.6);Extrude("R8_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(9.7092,14.9226,0.8,1.6);Extrude("R9_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(11.4908,11.6492,1.25,2);Extrude("Q1_dessous_Eagle",-1.05,1,false);
 Begin();Rect(13.98,11.6492,1.25,2);Extrude("Q2_dessous_Eagle",-1.05,1,false);
 Begin();Rect(15.8281,8.35246875,0.8,1.6);Extrude("R1_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(23.8972625,6.7857,1.6,0.8);Extrude("R3_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(15.8077,4.17416563,0.8,1.6);Extrude("R2_dessous_Eagle",-0.65,0.6,false);
 Begin();Rect(26.6192,1.8923,1.016,1.143);Extrude("Pad_U1_0",-.05,.05,false);
 Begin();Rect(24.8412,1.8923,1.016,1.143);Extrude("Pad_U1_1",-.05,.05,false);
 Begin();Rect(25.7302,3.9243,1.016,1.143);Extrude("Pad_U1_2",-.05,.05,false);
 Begin();Rect(25.84220625,7.4397,0.762,0.696);Extrude("Pad_C2_0",-.05,.05,false);
 Begin();Rect(25.84220625,6.2357,0.762,0.696);Extrude("Pad_C2_1",-.05,.05,false);
 Begin();Rect(12.0294,5.969,0.696,0.762);Extrude("Pad_C4_0",-.05,.05,false);
 Begin();Rect(13.2334,5.969,0.696,0.762);Extrude("Pad_C4_1",-.05,.05,false);
 Begin();Rect(27.36815,7.4397,0.762,0.696);Extrude("Pad_C1_0",-.05,.05,false);
 Begin();Rect(27.36815,6.2357,0.762,0.696);Extrude("Pad_C1_1",-.05,.05,false);
 Begin();Rect(12.0294,4.59316563,0.696,0.762);Extrude("Pad_C5_0",-.05,.05,false);
 Begin();Rect(13.2334,4.59316563,0.696,0.762);Extrude("Pad_C5_1",-.05,.05,false);
 Begin();Rect(7.874,17.2466,0.696,0.762);Extrude("Pad_C6_0",-.05,.05,false);
 Begin();Rect(6.67,17.2466,0.696,0.762);Extrude("Pad_C6_1",-.05,.05,false);
 Begin();Rect(10.414,17.2466,0.696,0.762);Extrude("Pad_C7_0",-.05,.05,false);
 Begin();Rect(9.21,17.2466,0.696,0.762);Extrude("Pad_C7_1",-.05,.05,false);
 Begin();Rect(16.83,17.2466,0.696,0.762);Extrude("Pad_C8_0",-.05,.05,false);
 Begin();Rect(18.034,17.2466,0.696,0.762);Extrude("Pad_C8_1",-.05,.05,false);
 Begin();Rect(19.37,17.2466,0.696,0.762);Extrude("Pad_C9_0",-.05,.05,false);
 Begin();Rect(20.574,17.2466,0.696,0.762);Extrude("Pad_C9_1",-.05,.05,false);
 Begin();Rect(16.83,15.3416,0.696,0.762);Extrude("Pad_R11_0",-.05,.05,false);
 Begin();Rect(18.034,15.3416,0.696,0.762);Extrude("Pad_R11_1",-.05,.05,false);
 Begin();Rect(12.954,17.2466,0.696,0.762);Extrude("Pad_R10_0",-.05,.05,false);
 Begin();Rect(11.75,17.2466,0.696,0.762);Extrude("Pad_R10_1",-.05,.05,false);
 Begin();Rect(15.2577,5.969,0.696,0.762);Extrude("Pad_C3_0",-.05,.05,false);
 Begin();Rect(16.4617,5.969,0.696,0.762);Extrude("Pad_C3_1",-.05,.05,false);
 Begin();Rect(15.786,7.3716,0.762,0.7);Extrude("Pad_D1_0",-.05,.05,false);
 Begin();Rect(14.186,7.3716,0.762,0.7);Extrude("Pad_D1_1",-.05,.05,false);
 Begin();Rect(5.08,12.66,2.54,1);Extrude("Pad_J2_0",-.05,.05,false);
 Begin();Rect(5.08,10.66,2.54,1);Extrude("Pad_J2_1",-.05,.05,false);
 Begin();Rect(5.08,8.66,2.54,1);Extrude("Pad_J2_2",-.05,.05,false);
 Begin();Rect(5.08,6.66,2.54,1);Extrude("Pad_J2_3",-.05,.05,false);
 Begin();Rect(1.8,14.96,2.6,2);Extrude("Pad_J2_4",-.05,.05,false);
 Begin();Rect(1.8,3.36,2.6,2);Extrude("Pad_J2_5",-.05,.05,false);
 Begin();Rect(12.0294,3.21733125,0.696,0.762);Extrude("Pad_R4_0",-.05,.05,false);
 Begin();Rect(13.2334,3.21733125,0.696,0.762);Extrude("Pad_R4_1",-.05,.05,false);
 Begin();Rect(15.2577,3.21733125,0.696,0.762);Extrude("Pad_R5_0",-.05,.05,false);
 Begin();Rect(16.4617,3.21733125,0.696,0.762);Extrude("Pad_R5_1",-.05,.05,false);
 Begin();Rect(12.954,15.3416,0.696,0.762);Extrude("Pad_R6_0",-.05,.05,false);
 Begin();Rect(11.75,15.3416,0.696,0.762);Extrude("Pad_R6_1",-.05,.05,false);
 Begin();Rect(6.67,15.3416,0.696,0.762);Extrude("Pad_R7_0",-.05,.05,false);
 Begin();Rect(7.874,15.3416,0.696,0.762);Extrude("Pad_R7_1",-.05,.05,false);
 Begin();Rect(15.494,17.2466,0.696,0.762);Extrude("Pad_R8_0",-.05,.05,false);
 Begin();Rect(14.29,17.2466,0.696,0.762);Extrude("Pad_R8_1",-.05,.05,false);
 Begin();Rect(10.3632,15.3416,0.696,0.762);Extrude("Pad_R9_0",-.05,.05,false);
 Begin();Rect(9.1592,15.3416,0.696,0.762);Extrude("Pad_R9_1",-.05,.05,false);
 Begin();Rect(11.1633,13.1572,0.635,0.762);Extrude("Pad_Q1_0",-.05,.05,false);
 Begin();Rect(12.4333,13.1572,0.635,0.762);Extrude("Pad_Q1_1",-.05,.05,false);
 Begin();Rect(11.7983,11.3792,0.635,0.762);Extrude("Pad_Q1_2",-.05,.05,false);
 Begin();Rect(13.6525,13.1572,0.635,0.762);Extrude("Pad_Q2_0",-.05,.05,false);
 Begin();Rect(14.9225,13.1572,0.635,0.762);Extrude("Pad_Q2_1",-.05,.05,false);
 Begin();Rect(14.2875,11.3792,0.635,0.762);Extrude("Pad_Q2_2",-.05,.05,false);
 Begin();Rect(16.4821,8.77146875,0.696,0.762);Extrude("Pad_R1_0",-.05,.05,false);
 Begin();Rect(15.2781,8.77146875,0.696,0.762);Extrude("Pad_R1_1",-.05,.05,false);
 Begin();Rect(24.3162625,6.2357,0.762,0.696);Extrude("Pad_R3_0",-.05,.05,false);
 Begin();Rect(24.3162625,7.4397,0.762,0.696);Extrude("Pad_R3_1",-.05,.05,false);
 Begin();Rect(16.4617,4.59316563,0.696,0.762);Extrude("Pad_R2_0",-.05,.05,false);
 Begin();Rect(15.2577,4.59316563,0.696,0.762);Extrude("Pad_R2_1",-.05,.05,false);
 Begin();Rect(15.748,1.524,1.016,1.016);Extrude("Pad_JP2_0",-.05,.05,false);
 Begin();Rect(13.208,1.524,1.016,1.016);Extrude("Pad_JP2_1",-.05,.05,false);
 Begin();Rect(14.478,1.524,1.016,1.016);Extrude("Pad_JP2_2",-.05,.05,false);
 Begin();Rect(21.717,1.524,1.016,1.016);Extrude("Pad_JP1_0",-.05,.05,false);
 Begin();Rect(19.177,1.524,1.016,1.016);Extrude("Pad_JP1_1",-.05,.05,false);
 Begin();Rect(20.447,1.524,1.016,1.016);Extrude("Pad_JP1_2",-.05,.05,false);
 Begin();Rect(21.717,3.048,1.016,1.016);Extrude("Pad_JP3_0",-.05,.05,false);
 Begin();Rect(19.177,3.048,1.016,1.016);Extrude("Pad_JP3_1",-.05,.05,false);
 Begin();Rect(20.447,3.048,1.016,1.016);Extrude("Pad_JP3_2",-.05,.05,false);
 Begin();Rect(21.717,4.572,1.016,1.016);Extrude("Pad_JP4_0",-.05,.05,false);
 Begin();Rect(19.177,4.572,1.016,1.016);Extrude("Pad_JP4_1",-.05,.05,false);
 Begin();Rect(20.447,4.572,1.016,1.016);Extrude("Pad_JP4_2",-.05,.05,false);
 Begin();Rect(21.717,6.096,1.016,1.016);Extrude("Pad_JP5_0",-.05,.05,false);
 Begin();Rect(19.177,6.096,1.016,1.016);Extrude("Pad_JP5_1",-.05,.05,false);
 Begin();Rect(20.447,6.096,1.016,1.016);Extrude("Pad_JP5_2",-.05,.05,false);
 Begin();Circle(37.0078,15.2908,.95);Circle(37.0078,15.2908,.5461);Extrude("Contact_SPI_0",1.6,.05,false);
 Begin();Circle(37.0078,12.7508,.95);Circle(37.0078,12.7508,.5461);Extrude("Contact_SPI_1",1.6,.05,false);
 Begin();Circle(37.0078,17.8308,.95);Circle(37.0078,17.8308,.5461);Extrude("Contact_SPI_2",1.6,.05,false);
 Begin();Circle(37.0078,10.2108,.95);Circle(37.0078,10.2108,.5461);Extrude("Contact_SPI_3",1.6,.05,false);
 Begin();Circle(37.0078,7.6708,.95);Circle(37.0078,7.6708,.5461);Extrude("Contact_SPI_4",1.6,.05,false);
 Begin();Circle(37.0078,5.1308,.95);Circle(37.0078,5.1308,.5461);Extrude("Contact_SPI_5",1.6,.05,false);
 Begin();Circle(37.0078,2.5908,.95);Circle(37.0078,2.5908,.5461);Extrude("Contact_SPI_6",1.6,.05,false);

 object[] bodies=(object[])((IPartDoc)doc).GetBodies2(0,false);int pcb=0,screen=0;
 foreach(IBody2 b in bodies){double[] bb=(double[])b.GetBodyBox();double dx=bb[3]-bb[0],dy=bb[4]-bb[1],dz=bb[5]-bb[2];double[] c;
  if(dx>.039){c=new double[]{.02,.35,.49,1,.7,.2,.2,0,0};b.Name="PCB_104020249_source_v1_0";pcb++;}
  else if(dx>.021 && dx<.022 && bb[2]>.0039){c=new double[]{.045,.055,.09,1,.7,.4,.3,0,0};screen++;}
  else if(dx>.024){c=new double[]{.10,.08,.12,1,.7,.4,.3,0,0};}
  else if(bb[0]<.0011 && dy>.012){c=new double[]{.92,.91,.86,1,.7,.1,.1,0,0};}
  else if(dz<.0002 || dz>.005){c=new double[]{.75,.73,.67,1,.6,.6,.4,0,0};}
  else c=new double[]{.13,.12,.13,1,.7,.1,.1,0,0};
  b.MaterialPropertyValues2=c;
 }
 if(pcb!=1 || screen!=1 || bodies.Length<80)throw new Exception("Geometry check failed: "+bodies.Length);
 doc.ForceRebuild3(false);doc.ShowNamedView2("*Isometric",7);doc.ViewZoomtofit2();
 int err=0,warn=0;string target=Path.Combine(folder,"Grove_OLED_Yellow_Blue_104020249.SLDPRT");
 if(!doc.Extension.SaveAs(target,0,1,null,ref err,ref warn))throw new Exception("Save failed "+err);
 Console.WriteLine("Saved "+target+"; bodies="+bodies.Length+"; warnings="+warn);
 doc.SaveBMP(Path.Combine(folder,"apercu.bmp"),1200,900);
 }
}
'@
[OLED249Model]::Run($PSScriptRoot)

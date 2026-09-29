$ErrorActionPreference='Stop'
$dll='C:\Program Files\SOLIDWORKS Corp\SOLIDWORKS\SolidWorks.Interop.sldworks.dll'
Add-Type -Path $dll
Add-Type -ReferencedAssemblies $dll -TypeDefinition @'
using System;
using System.IO;
using SolidWorks.Interop.sldworks;
public class ScdImport {
 public static void Run(string folder) {
 ISldWorks sw=(ISldWorks)Activator.CreateInstance(Type.GetTypeFromProgID("SldWorks.Application"));
 string path=Path.Combine(folder,@"sensirion_cad\Sensirion_CO2_Sensors_SCD30_POD_STEP_File.step");
 int err=0;
 IModelDoc2 doc=(IModelDoc2)sw.LoadFile4(path,"r",null,ref err);
 if(doc==null) throw new Exception("Import error "+err);
 Console.WriteLine("Type="+doc.GetType()+" title="+doc.GetTitle());
 try {object[] bs=(object[])((IPartDoc)doc).GetBodies2(0,false);foreach(IBody2 b in bs) Console.WriteLine(b.Name+" "+String.Join(",",(double[])b.GetBodyBox()));} catch(Exception e) {Console.WriteLine(e.Message);}
 doc.ShowNamedView2("*Isometric",7);doc.ViewZoomtofit2();doc.SaveBMP(Path.Combine(folder,"import.bmp"),1400,900);
 }
}
'@
[ScdImport]::Run($PSScriptRoot)

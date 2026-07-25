/* EASE R10.36 — protocole Web Serial minimal.
   Aucun accès SD, aucune lecture capteur forcée : STATUS lit le cache Arduino. */
const EaseSerial=(()=>{
  const BAUD=115200; let port=null,reader=null,writer=null,running=false,buf='';
  const delay=ms=>new Promise(r=>setTimeout(r,ms));
  const line=()=>{const i=buf.indexOf('\n');if(i<0)return null;const s=buf.slice(0,i).replace(/\r$/,'').trim();buf=buf.slice(i+1);return s};
  async function pump(){const dec=new TextDecoder();try{while(running){const {value,done}=await reader.read();if(done)break;buf+=dec.decode(value,{stream:true});}}catch(_){}}
  async function wait(pred,label,ms=2600){const end=Date.now()+ms;while(Date.now()<end){const s=line();if(s!==null){if(s.startsWith('#ERR'))throw new Error(s);if(pred(s))return s}await delay(10)}throw new Error('Timeout — réponse attendue : '+label)}
  async function send(s){buf='';await writer.write(new TextEncoder().encode(s+'\n'))}
  async function connect(){if(!('serial' in navigator))throw new Error('Web Serial requiert Chrome ou Edge');port=await navigator.serial.requestPort();await port.open({baudRate:BAUD});writer=port.writable.getWriter();reader=port.readable.getReader();running=true;pump();await delay(400);for(let i=0;i<8;i++){await send('P');try{await wait(s=>s==='PONG','PONG',700);return}catch(_){}}throw new Error('PONG absent : fermer le Moniteur série et recommencer')}
  async function disconnect(){running=false;try{if(reader){await reader.cancel();reader.releaseLock()}if(writer)writer.releaseLock();if(port)await port.close()}catch(_){}port=reader=writer=null;buf=''}
  async function status(){await send('S');return (await wait(s=>s.startsWith('#S;'),'#S;')).slice(3).split(';').map(Number)}
  async function config(){await send('G');const p=(await wait(s=>s.startsWith('#C;'),'#C;')).slice(3).split(';');return {interval:Number(p[0]),name:p[1]||'EASE-UNO',lat:p[2]||'',lon:p[3]||'',locationSet:p[4]==='1',rotation:p[5]==='M'?'MONTH':(p[5]==='S'?'SIZE':'WEEK')}}
  async function ok(cmd,timeout=2600){await send(cmd);await wait(s=>s==='#OK','#OK',timeout)}
  return {connect,disconnect,status,config,ok,connected:()=>!!port};
})();

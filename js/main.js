let api = {};

async function onUploadPlugin(e) {
  const file_data = new Uint8Array(await e.target.files[0].arrayBuffer());
  const f = Module.FS.open('plugin.wasm', 'w');
  Module.FS.write(f, file_data, 0, file_data.length);
  Module.FS.close(f);

  const ret_ptr = Module._malloc(4 + 4);
  console.log(ret_ptr);
  const status = api.test(ret_ptr);
  console.log(status);
  const point_count = new DataView(Module.HEAP8.buffer, ret_ptr, 4).getInt32(0, true);
  console.log(point_count);
  const point_position_data_ptr = new DataView(Module.HEAP8.buffer, ret_ptr + 4, 4).getUint32(0, true);
  console.log(point_position_data_ptr);
  const point_position_data = new Float32Array(Module.HEAP8.buffer, point_position_data_ptr, 3 * point_count);
  console.log(point_position_data);
}

Module.onRuntimeInitialized = async () => {
  console.log("Ready");
  api = {
    test: Module._test,
  }
  document.getElementById('plugin-input').addEventListener('change', onUploadPlugin);
};


// Three scene

function onLoaded() {
  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera( 45, window.innerWidth / window.innerHeight, 0.1, 1000 );

  const renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(window.innerWidth / 2, window.innerHeight / 2);
  document.body.appendChild(renderer.domElement);

  const controls = new THREE.OrbitControls( camera, renderer.domElement );

  camera.position.set(5, 3, 3);
  controls.update();

  const geometry = new THREE.BoxGeometry( 1, 1, 1 );
  const material = new THREE.MeshStandardMaterial( { color: 0x00ff00 } );
  const cube = new THREE.Mesh( geometry, material );
  scene.add( cube );

  const directionalLight = new THREE.DirectionalLight( 0xffffff, 1.0 );
  directionalLight.position.set( 100, 200, 300 );
  scene.add( directionalLight );
  const directionalLight2 = new THREE.DirectionalLight( 0xffffff, 0.3 );
  directionalLight2.position.set( -300, -100, -200 );
  scene.add( directionalLight2 );

  function animate() {
    requestAnimationFrame( animate );
    controls.update();
    renderer.render( scene, camera );
  }
  animate();
}
document.addEventListener('DOMContentLoaded', onLoaded);

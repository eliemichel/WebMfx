let api = {};

function App() {
  this.needRender = true;

  this.onAnimationFrame = this.onAnimationFrame.bind(this);
  this.onCameraMoved = this.onCameraMoved.bind(this);
  this.onDomLoaded = this.onDomLoaded.bind(this);
  this.onLoadPlugin = this.onLoadPlugin.bind(this);
}

App.prototype.onDomLoaded = function() {
  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera( 45, window.innerWidth / window.innerHeight, 0.1, 1000 );

  const renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(window.innerWidth / 2, window.innerHeight / 2);
  document.body.appendChild(renderer.domElement);

  const controls = new THREE.OrbitControls( camera, renderer.domElement );

  camera.position.set(5, 3, 3);
  controls.update();

  const geometry = new THREE.BoxGeometry(1, 1, 1);
  const material = new THREE.MeshStandardMaterial( { color: 0x00ff00 } );
  const cube = new THREE.Mesh( geometry, material );
  scene.add( cube );

  {
    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute( 'position', new THREE.Float32BufferAttribute( [], 3 ) );
    const material = new THREE.PointsMaterial( { color: 0x0088ff, size: 0.5 } );
    const points = new THREE.Points( geometry, material );
    scene.add( points );
    this.pointGeometry = geometry;
  }

  const directionalLight = new THREE.DirectionalLight( 0xffffff, 1.0 );
  directionalLight.position.set( 100, 200, 300 );
  scene.add( directionalLight );
  const directionalLight2 = new THREE.DirectionalLight( 0xffffff, 0.3 );
  directionalLight2.position.set( -300, -100, -200 );
  scene.add( directionalLight2 );

  controls.addEventListener('change', this.onCameraMoved);

  this.controls = controls;
  this.renderer = renderer;
  this.scene = scene;
  this.camera = camera;
  this.onAnimationFrame();
}

App.prototype.onAnimationFrame = function() {
  requestAnimationFrame(this.onAnimationFrame);

  this.controls.update();

  if (this.needRender) {
    this.renderer.render(this.scene, this.camera);
    this.needRender = false;
  }
}

App.prototype.onCameraMoved = function() {
  this.needRender = true;
}

App.prototype.onLoadPlugin = async function(event) {
  const file_data = new Uint8Array(await event.target.files[0].arrayBuffer());
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

  // Update geometry
  this.pointGeometry.setAttribute('position', new THREE.BufferAttribute(point_position_data, 3));
  this.needRender = true;
};

const app = new App();

Module.onRuntimeInitialized = async () => {
  console.log("WebAssembly module is ready.");
  api = {
    test: Module._test,
  }
  document.getElementById('plugin-input').addEventListener('change', app.onLoadPlugin);
};

document.addEventListener('DOMContentLoaded', app.onDomLoaded);

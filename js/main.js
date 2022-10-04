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
    //const material = new THREE.PointsMaterial( { color: 0x0088ff, size: 0.5 } );
    //const points = new THREE.Points( geometry, material );
    const material = new THREE.MeshStandardMaterial( { color: 0x0088ff } );
    const points = new THREE.Mesh( geometry, material );
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

  const ret_ptr = Module._malloc(7 * 4);
  console.log(ret_ptr);
  const status = api.test(ret_ptr);
  console.log(status);
  const view = new DataView(Module.HEAP8.buffer, ret_ptr, 7 * 4);
  const point_count = view.getInt32(0 * 4, true);
  const corner_count = view.getInt32(1 * 4, true);
  const face_count = view.getInt32(2 * 4, true);
  const constant_face_size = view.getInt32(3 * 4, true);
  console.log(point_count);
  console.log(corner_count);
  console.log(face_count);
  console.log(constant_face_size);

  const point_position_data_ptr = view.getUint32(4 * 4, true);
  console.log(point_position_data_ptr);
  const point_position_data = new Float32Array(Module.HEAP8.buffer, point_position_data_ptr, 3 * point_count);
  console.log(point_position_data);

  const corner_point_data_ptr = view.getUint32(5 * 4, true);
  console.log(corner_point_data_ptr);
  const corner_point_data = new Int32Array(Module.HEAP8.buffer, corner_point_data_ptr, corner_count);
  console.log(corner_point_data);

  let face_size_data;
  if (constant_face_size == -1) {
    const face_size_data_ptr = view.getUint32(6 * 4, true);
    console.log(face_size_data_ptr);
    face_size_data = new Int32Array(Module.HEAP8.buffer, face_size_data_ptr, face_count);
  } else {
    face_size_data = new Int32Array(face_count);
    face_size_data.fill(constant_face_size);
  }
  console.log(face_size_data);

  let triangle_count = 0;
  for (let f = 0 ; f < face_count ; ++f) {
    const size = face_size_data[f];
    if (size == 3) {
      triangle_count += 1;
    } else if (size == 4) {
      triangle_count += 2;
    } else {
      console.error(`Unsupported face size in face #${f}: ${size}`);
    }
  }
  const tesselated_corner_point_data = new Uint32Array(3 * triangle_count);
  let triangle_index = 0;
  let corner_index = 0;
  for (let f = 0 ; f < face_count ; ++f) {
    const size = face_size_data[f];

    for (let k = 0 ; k < 3 ; ++k) {
      tesselated_corner_point_data[3 * triangle_index + k] = corner_point_data[corner_index + k];
    }
    ++triangle_index;

    if (size == 4) {
      for (let k = 0 ; k < 3 ; ++k) {
        tesselated_corner_point_data[3 * triangle_index + k] = corner_point_data[corner_index + (k + 2) % 4];
      }
      ++triangle_index;
    }

    corner_index += size;
  }

  // Expand the index buffer and compute normals
  const tesselated_point_position_data = new Float32Array(3 * 3 * triangle_count);
  const tesselated_point_normal_data = new Float32Array(3 * 3 * triangle_count);
  const pos = [new THREE.Vector3(), new THREE.Vector3(), new THREE.Vector3()];
  const ab = new THREE.Vector3();
  const ac = new THREE.Vector3();
  const normal = new THREE.Vector3();
  for (let t = 0 ; t < triangle_count ; ++t) {
    for (let c = 0 ; c < 3 ; ++c) {
      const p = tesselated_corner_point_data[3 * t + c];
      for (let k = 0 ; k < 3 ; ++k) {
        const value = point_position_data[3 * p + k];
        pos[c].setComponent(k, value);
        tesselated_point_position_data[3 * (3 * t + c) + k] = value;
      }
    }
    ab.subVectors(pos[1], pos[0]);
    ac.subVectors(pos[2], pos[0]);
    normal.crossVectors(ab, ac);
    normal.normalize();
    for (let c = 0 ; c < 3 ; ++c) {
      for (let k = 0 ; k < 3 ; ++k) {
        tesselated_point_normal_data[3 * (3 * t + c) + k] = normal.getComponent(k);
      }
    }
  }

  // Update geometry
  this.pointGeometry.setAttribute('position', new THREE.BufferAttribute(tesselated_point_position_data, 3));
  this.pointGeometry.setAttribute('normal', new THREE.BufferAttribute(tesselated_point_normal_data, 3));
  //this.pointGeometry.setIndex(new THREE.BufferAttribute(tesselated_corner_point_data, 1));
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
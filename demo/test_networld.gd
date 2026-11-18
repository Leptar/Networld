extends Node

var views: Dictionary = {} # entity_id (int) -> MeshInstance3D
var vbox: VBoxContainer
var is_client: bool = false
var camera: Camera3D
var local_entity_id: int = 0

func _ready() -> void:
	# 1. Caméra avec rotation statique
	camera = Camera3D.new()
	camera.position = Vector3(0, 7, 10)
	camera.rotation_degrees = Vector3(-35, 0, 0)
	add_child(camera)

	# 2. Connexion aux signaux réseau
	Networld.entity_spawned.connect(_on_entity_spawned)
	Networld.entity_despawned.connect(_on_entity_despawned)

	# 3. Interface Utilisateur de test
	vbox = VBoxContainer.new()
	add_child(vbox)

	var btn_host := Button.new()
	btn_host.text = "Démarrer en tant que HOST"
	btn_host.pressed.connect(_on_host_pressed)
	vbox.add_child(btn_host)

	var btn_client := Button.new()
	btn_client.text = "Démarrer en tant que CLIENT"
	btn_client.pressed.connect(_on_client_pressed)
	vbox.add_child(btn_client)

func _on_host_pressed() -> void:
	add_child(Networld)
	Networld.host(7070)
	print("[HOST] Démarré !")
	vbox.hide()

func _on_client_pressed() -> void:
	is_client = true
	add_child(Networld)
	Networld.join("127.0.0.1", 7070)
	print("[CLIENT] Connecté !")
	vbox.hide()

func _on_entity_spawned(entity_id: int, pos: Vector3, is_local: bool) -> void:
	if views.has(entity_id):
		return

	var cube := MeshInstance3D.new()
	cube.mesh = BoxMesh.new()
	cube.position = pos
	add_child(cube)
	
	if (is_local):
		local_entity_id = entity_id
		var mat = StandardMaterial3D.new()
		mat.albedo_color = Color(0.2, 0.6, 1.0)
		mat.roughness = 0.4
		mat.metallic = 0.1
		cube.material_override = mat
		camera.reparent(cube, false)

	views[entity_id] = cube
	Networld.bind_entity_view(entity_id, cube)
	print("[VIEW] Spawn entité ", entity_id, " à ", pos)

func _on_entity_despawned(entity_id: int) -> void:
	if not views.has(entity_id):
		return
		
	if entity_id == local_entity_id:
		local_entity_id = 0
		if is_instance_valid(camera) and camera.get_parent():
			camera.reparent(self, false)
	
	var cube: MeshInstance3D = views[entity_id]
	views.erase(entity_id)
	if is_instance_valid(cube):
		cube.queue_free()

	print("[VIEW] Despawn entité ", entity_id)

func _process(_delta: float) -> void:
	if not is_client:
		return
	
	var dir: Vector2 = Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	Networld.set_client_input(Vector3(dir.x, 0.0, dir.y))

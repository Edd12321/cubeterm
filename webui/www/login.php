<?php
include 'util.php';
if (session_status() == PHP_SESSION_NONE)
	session_start();
if (isset($_SESSION["logged-in"])) {
	header("Location: index.php");
	exit();
}
?>
<!DOCTYPE HTML>
<html>
<head>
<?php
mobile_view();
?>
<link rel="stylesheet" href="styles.css" />
</head>
<body>
<?php
navbar();
?>
<form action="login.php" class="center" method="post">
<table class="form-box">
	<tr>
		<th><h2>Welcome to Cubeterm WebUI!</h2></th>
	</tr><tr>
		<td><label for="name">Name:</label></td>
		<td><input type="text" disabled name="name" value="<?=get_current_user();?>" /></td>
	</tr><tr>
		<td><label for="pass">Password: </label></td>
		<td><input type="password" required name="pass" /></td>
	</tr><tr>
		<td><button type="submit">Login</button></td>
	</tr>
<?php
if ($_SERVER["REQUEST_METHOD"] === "POST") {
	if (md5($_POST["pass"]) != file("../password.md5", FILE_IGNORE_NEW_LINES)[0])
		complain("Wrong password!");
	$_SESSION["logged-in"] = true;
	refresh();
}
?>
</table>
</form>
</body>

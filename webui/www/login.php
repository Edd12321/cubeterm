<!DOCTYPE HTML>
<html>
<head>
<link rel="stylesheet" href="styles.css" />
</head>
<body>

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
include 'util.php';

// Aici incepe distractia
if ($_SERVER["REQUEST_METHOD"] === "POST") {
	if (md5($_POST["pass"]) != file("../password.md5", FILE_IGNORE_NEW_LINES)[0])
		complain("Wrong password!");
	session_start();
	$_SESSION["logged-in"] = true;
	header("Location: index.php");
}
?>
</table>
</form>
</body>

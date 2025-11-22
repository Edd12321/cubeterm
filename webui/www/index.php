<?php
include 'util.php';
if (session_status() == PHP_SESSION_NONE)
	session_start();
if (!isset($_SESSION["logged-in"])) {
	header("Location: login.php");
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
<script src="https://animcubejs.cubing.net/AnimCube3.js"></script>
</head>
<body>

<?php
navbar();
?>
<br />
<br />
<form action="index.php" method="post" id="main-form">
<center>
<table class="form-box">
<tr>
	<th><h2>Upload form</h2></th>
	<tr>
		<td><label for="method">Method: </label>
		</td>
		<td>
<?php
			$files = glob("res/*.png");
			foreach ($files as $i => $path) {
				$name = basename($path, ".png");
				echo "\t\t<div class=\"method-selector\">
			<img src=\"$path\" />
			<input type=\"radio\" name=\"method\" value=\"$name\"";
				if ($i + 1 == sizeof($files))
					echo " required checked=\"checked\"";
				echo " />$name\n\t\t</div>\n";
				if ($i && $i % 3 == 0)
					echo "\t\t<br />\n";
			}
?>
		</td>
	</tr>
	<tr>
		<td><label for="scram">Scramble: </label>
		</td>
		<td><textarea name="scram" form="main-form" placeholder="Leave this empty to get a random scramble..."></textarea>
		</td>
	</tr>
	<tr>
		<td><label for="scram-len">Random scramble length:</label>
		</td>
		<td><input type="text" name="scram-len" value="15" />
		</td>
	</tr>
	<tr>
		<td><label for="solve-name">Name: </label>
		</td>
		<td><input type="text" name="solve-name" required />
			<button type="submit">Solve the cube!</button>
		</td>
	</tr>
<?php
@ob_flush();
flush();
if ($_SERVER["REQUEST_METHOD"] === "POST") {
	if (isset($_POST["delete"])) {
		$del = $_POST["delete"];
		$pid = file_get_contents("$del/pid.txt");
		if (posix_getpgid($pid))
			posix_kill($pid, SIGKILL);
		@unlink("$del/out.txt");
		@unlink("$del/pid.txt");
		rmdir($del);
	} else {

		$method = $_POST["method"];
		$scram = $_POST["scram"];
		$scram_len = $_POST["scram-len"];
		$name = $_POST["solve-name"];

		if (!in_array($method, [ "ZZ", "CFOP", "Roux", "Petrus", "2GR", "Mehta" ]))
			complain("Invalid method $method!");
		if (empty($name))
			complain("Must provide a name!");
		if (strpos($scram, '\n') !== FALSE)
			complain("Scramble must be on a single line!");
		if (preg_match("/[^a-z_\-0-9]/i", $name))
			complain("Name must contain only alphanumeric chars, hyphens or underscores");

		$args = [ null, null, "-m", $method, "-o", "out.txt" ];
		if (ctype_digit($scram_len) && $scram_len > 0 && empty($scram)) {
			$args[0] = "-r";
			$args[1] = $scram_len;
		} else if (!empty($scram)) {
			$args[0] = "-s";
			$args[1] = $scram;
		} else {
			complain("Please provide a valid scramble!");
		}
		if (pcntl_fork() == 0) {
			$dir = "../jobs/$method.$name.".time();
			mkdir($dir);
			chdir($dir);
			file_put_contents("pid.txt", getmypid());
			pcntl_exec("../../../bin/cubeterm", $args);
			exit();
		}
		refresh();
	}
}

while (pcntl_waitpid(-1, $status, WNOHANG|WUNTRACED) > 0)
;

?>
</table>
</center>
</form>

<hr />
<div id="solves-list">
<?php
$solves = glob("../jobs/*/", GLOB_NOSORT);
usort($solves, function($x, $y) {
	return filemtime($y) - filemtime($x);
});
foreach ($solves as $solve) {
	echo '<div class="form-box">';

	$s = explode('.', basename($solve));
	echo "[<b>$s[0]</b>]";
	echo "[".date("Y-m-d H:i", filemtime($solve))."] ";
	echo "<i>$s[1]</i> ";
	echo "<hr />";

	$pid = file_get_contents("$solve/pid.txt");
	if (posix_getpgid($pid)) {
		echo "<b><font color=green>(Solving...)</font></b><br />";
		echo "<b><font color=blue>PID: $pid</font></b>";
	} else {
		$out = file("$solve/out.txt");

		/* Scramble: */
		$scram = trim(explode(": ", $out[0])[1]);
		echo "<b>Scramble: </b> <br /> <code>$scram</code> <br />";

		/* Solution: */
		echo "<b>Solution:</b>";
		echo "<div class='sol-view'>";
		echo "<pre>";

		$sl = '';
		for ($i = 10; $i < count($out)-5; ++$i)
			$sl .= $out[$i];
		echo "$sl</pre>";

		$sl = preg_replace("|//(.+)\n|", " ", $sl);
		echo "<div class='cube' style='width:160px;height:179px'><script>AnimCube3(\"wca=1&initmove=y x2 $scram&move=$sl\")</script></div>";
		echo "</div>";
	}
	echo "
		<form method=post action=index.php>
			<button name=delete value=$solve type=submit>Delete</button>
		</form></div>";
}
?>
</div>

</body>
</html>

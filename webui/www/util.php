<?php
function mobile_view()
{
	echo "<meta name='viewport' content='initial-scale=0.65,maximum-scale=0.65,minimum-scale=0.65,user-scalable=no' />";
}

function navbar()
{
	echo "<ul id='navbar'>";
	echo isset($_SESSION["logged-in"]) 
		? "<li><a href='logout.php'>Log out</a></li>"
		: "<li><a href='login.php'>Log in</a></li>";
	echo "<li><a href='https://github.com/Edd12321/cubeterm'>GitHub</a></li>";
	echo "</ul>";
}

function refresh()
{
	echo "<meta http-equiv='refresh' content='0' />";
}

function complain($message)
{
	echo 
		"<tr><td colspan=3>
		<hr />
		<font color=red>
			<b>$message</b>
		</font>
		</td></tr>";
	exit();
}
?>

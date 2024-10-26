<?php
function complain($message)
{
	echo 
		"<tr><td colspan=3>
		<hr />
		<font color=red>
			<b>
				$message
			</b>
		</font>
		</td></tr>";
	exit();
}

?>

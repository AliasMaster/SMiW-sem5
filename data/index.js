setInterval(async function () {
  const res = await fetch('/temp');
  const data = await res.text();
  console.log(data);
  document.querySelector('#temp').textContent = data;
}, 1000);

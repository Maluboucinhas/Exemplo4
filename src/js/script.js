//JOGO DA ADVINHAÇÃO 

let palpite;

const sorteio=Math.floor(Math.random()* 10)+1 

do{
    palpite =parseInt(prompt("Escolha um número entre 1 e 10"));
    if(palpite !== sorteio)
        alert("Você perdeu 100 reais")

} while(palpite !== sorteio)
    alert(`Parabéns, você acertou o numero é ${palpite}`)

